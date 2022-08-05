#ifndef SIMPLIFY_H
#define SIMPLIFY_H
#include <Eigen/core>
#include <iostream>
#include <vector>

namespace lim {
	namespace n_simp {
		static struct Triangle { 
			int v[3];
			float err[4];
			int deleted,dirty;
			glm::vec3 n;
		};
		struct Vertex {
			glm::vec3 p;
			int tstart,tcount;
			 Eigen::Matrix4f q;
			int border;
		};
		struct Ref { 
			int tid,tvertex; 
		}; 
	}
}

namespace {
	std::vector<lim::n_simp::Vertex> vertices;
	std::vector<lim::n_simp::Triangle> triangles;
	std::vector<lim::n_simp::Ref> refs;

	void updateGlobal(const lim::Mesh* mesh) {
		for(const lim::n_model::Vertex& mv : mesh->vertices) {
			lim::n_simp::Vertex v;
			v.p = mv.p;
			vertices.push_back(v);
		}
		if(mesh->indices.size()%3!=0) {
			fprintf(stderr, "simplify failed : mesh is not triangle mesh\n");
		}
		int numOfTri = mesh->indices.size()/3;
		for(int i=0; i<numOfTri; i++) {
			lim::n_simp::Triangle tri;
			for(int j=0; j<3; j++) {
				int ti = i*3 + j;
				tri.v[j] = mesh->indices[ti]; 
			}
			triangles.push_back(tri);
		}
		printf("simplify input %d triangles, %d vertices\n", triangles.size(), vertices.size());
	}


	// Error between vertex and Quadric

	double vertex_error(const Eigen::Matrix4f& q, const int x, const int y, const int z)
	{
		//모든평면 p까지의 거리
		// vt * Q * v 
		// Eigen::Vector3f ev(x, y, z);
		//return ev.transpose() * q * ev;
 		return   q[0]*x*x + 2*q[1]*x*y + 2*q[2]*x*z + 2*q[3]*x
				+ q[4]*y*y + 2*q[5]*y*z + 2*q[6]*y + q[7]*z*z + 2*q[8]*z + q[9];
	}

	double m_det( int a11, int a12, int a13
						, int a21, int a22, int a23
						, int a31, int a32, int a33, const Eigen::Matrix4f& m ) {
		double det = m[a11]*m[a22]*m[a33] - m[a11]*m[a23]*m[a32]
					+m[a12]*m[a23]*m[a32] - m[a12]*m[a21]*m[a33]
					+m[a13]*m[a21]*m[a32] - m[a13]*m[a22]*m[a31];
		return det;
	}

	// Error for one edge

	double calculate_error(int id_v1, int id_v2, glm::vec3& p_result)
	{
		// compute interpolated vertex 

		Eigen::Matrix4f q = vertices[id_v1].q + vertices[id_v2].q;
		bool   border = vertices[id_v1].border & vertices[id_v2].border;
		double error=0;
		double det = m_det( 0, 1, 2, 
							1, 4, 5,
							2, 5, 7, q );

		if ( det != 0 && !border )
		{
			// q_delta is invertible
			p_result.x = -1/det*m_det(1, 2, 3, 4, 5, 6, 5, 7, 8, q);	// vx = A41/det(q_delta) 
			p_result.y =  1/det*m_det(0, 2, 3, 1, 5, 6, 2, 7, 8, q);	// vy = A42/det(q_delta) 
			p_result.z = -1/det*m_det(0, 1, 3, 1, 4, 6, 2, 5, 8, q);	// vz = A43/det(q_delta) 
			error = vertex_error(q, p_result.x, p_result.y, p_result.z);
		}
		else
		{
			// det = 0 -> try to find best result
			glm::vec3 p1=vertices[id_v1].p;
			glm::vec3 p2=vertices[id_v2].p;
			glm::vec3 p3=(p1+p2)/(float)2;
			double error1 = vertex_error(q, p1.x,p1.y,p1.z);
			double error2 = vertex_error(q, p2.x,p2.y,p2.z);
			double error3 = vertex_error(q, p3.x,p3.y,p3.z);
			error = glm::min(error1, glm::min(error2, error3));
			if (error1 == error) p_result=p1;
			if (error2 == error) p_result=p2;
			if (error3 == error) p_result=p3;
		}
		return error;
	}

	// compact triangles, compute edge error and build reference list

	void update_mesh(int iteration)
	{
		// compact triangles
		if(iteration>0) {
			int dst=0;
			for(int i=0; i<triangles.size(); i++) {
				if(!triangles[i].deleted) {
					triangles[dst++]=triangles[i];
				}
				triangles.resize(dst);
			}
		}

		// Init Reference ID list
		for(int i=0; i<vertices.size(); i++) {
			vertices[i].tstart=0;
			vertices[i].tcount=0;
		}
		// v per 삼각형개수 업데이트
		for(int i=0; i<triangles.size(); i++) {
			lim::n_simp::Triangle& t=triangles[i];
			for(int j=0; j<3; j++) {
				vertices[t.v[j]].tcount++;
			}
		}

		int tstart=0;
		for(int i=0; i<vertices.size(); i++) {
			lim::n_simp::Vertex& v=vertices[i];
			v.tstart=tstart;
			tstart+=v.tcount;
			v.tcount=0;
		}
		// Write References shared vert에서 vert array로
		refs.resize(triangles.size()*3);
		for(int i=0; i<triangles.size(); i++) {
			lim::n_simp::Triangle& t = triangles[i];
			for(int j=0; j<3; j++) {
				lim::n_simp::Vertex& v = vertices[t.v[j]];
				refs[v.tstart+v.tcount].tid=i;
				refs[v.tstart+v.tcount].tvertex=j;
				v.tcount++;
			}
		}

		// Init Quadrics by Plane & Edge Errors
		//
		// required at the beginning ( iteration == 0 )
		// recomputing during the simplification is not required,
		// but mostly improves the result for closed meshes
		//
		if( iteration == 0 ) {
			// Identify boundary : vertices[].border=0,1

			std::vector<int> vcount,vids;

			for(int i=0; i<vertices.size(); i++)
				vertices[i].border=0;

			for(int i=0; i<vertices.size(); i++) {
				lim::n_simp::Vertex& v = vertices[i];
				vcount.clear();
				vids.clear();
				for(int j=0; j<v.tcount; j++) {
					int k=refs[v.tstart+j].tid;
					lim::n_simp::Triangle& t=triangles[k];
					for(int k=0; k<3; k++) {
						int ofs=0, id=t.v[k];
						while(ofs<vcount.size()) {
							if(vids[ofs]==id)break;
							ofs++;
						}
						if(ofs==vcount.size()) {
							vcount.push_back(1);
							vids.push_back(id);
						}
						else {
							vcount[ofs]++;
						}
					}
				}
				for(int j=0; j<vcount.size(); j++) {
					if(vcount[j]==1)
						vertices[vids[j]].border=1;
				}
			}
			//initialize errors
			for(int i=0; i<vertices.size(); i++) {
				vertices[i].q = Eigen::Matrix4f::Zero();
			}

			for(int i=0; i<triangles.size(); i++) {
				lim::n_simp::Triangle& t = triangles[i];
				glm::vec3 n,p[3];
				for(int j=0; j<3; j++) {
					p[j]=vertices[t.v[j]].p;
				}
				n = glm::cross(p[1]-p[0], p[2]-p[0]);
				n = glm::normalize(n); // 평면의 노멀, 방정식의 abc
				float a=n.x, b=n.y, c=n.z;
				float d = glm::dot(-n, p[0]); // 평면과의 거리 d
				t.n=n;
				Eigen::Matrix4f sym;
				sym << a*a,a*b,a*c,a*d,
					   b*a,b*b,b*c,b*d,
					   c*a,c*b,c*c,c*d,
					   d*a,d*b,d*c,d*d;
				for(int j=0; j<3; j++) {
					vertices[t.v[j]].q = vertices[t.v[j]].q + sym;
				}
			}
			for(int i=0; i<triangles.size(); i++) {
				// Calc Edge Error
				lim::n_simp::Triangle &t=triangles[i];
				glm::vec3 p;
				for(int j=0; j<3; j++) {
					t.err[j]=calculate_error(t.v[j],t.v[(j+1)%3],p);
				}
				t.err[3]=glm::min(t.err[0],glm::min(t.err[1],t.err[2]));
			}
		}
	}

	bool flipped( const glm::vec3& p, int i0, int i1
				, const lim::n_simp::Vertex& v0, const lim::n_simp::Vertex& v1
				, std::vector<int>& deleted ) 
	{
		int bordercount = 0;

		for(int k=0; k<v0.tcount; k++) {
			lim::n_simp::Triangle& t = triangles[refs[v0.tstart+k].tid];
			if(t.deleted)
				continue;
			int s=refs[v0.tstart+k].tvertex;
			int id1=t.v[(s+1)%3];
			int id2=t.v[(s+2)%3];

			if(id1==i1 || id2==i1) { // ??
				bordercount++;
				deleted[k]=1;
				continue;
			} 
			glm::vec3 d1 = vertices[id1].p-p;
			d1=glm::normalize(d1);
			glm::vec3 d2 = vertices[id2].p-p;
			d2=glm::normalize(d2);
			if(fabs(glm::dot(d1,d2)>0.999))
				return true;
			glm::vec3 n;
			n=glm::cross(d1,d2);
			n=glm::normalize(n);
			deleted[k]=0;
			if(glm::dot(n, t.n)<0.2)return true;
		}
		return false;
	}
	void update_triangles( int i0, lim::n_simp::Vertex& v,
						   std::vector<int>& deleted, int& deleted_triangles)
	{
		glm::vec3 p;
		for(int k=0; k<v.tcount; k++) {
			lim::n_simp::Ref& r = refs[v.tstart+k];
			lim::n_simp::Triangle& t = triangles[r.tid]; 
			if(t.deleted)
				continue;
			if(deleted[k]) {
				t.deleted=1;
				deleted_triangles++;
				continue;
			}
			t.v[r.tvertex]=i0; //??
			t.dirty=1;
			t.err[0]=calculate_error(t.v[0],t.v[1],p);
			t.err[1]=calculate_error(t.v[1],t.v[2],p);
			t.err[2]=calculate_error(t.v[2],t.v[0],p);
			t.err[3]=glm::min(t.err[0],glm::min(t.err[1],t.err[2]));
			refs.push_back(r);
		}
	}

	// Finally compact mesh before exiting

	void compact_mesh()
	{
		int dst=0;
		for(int i=0; i<vertices.size(); i++)
		{
			vertices[i].tcount=0;
		}
		for(int i=0; i<triangles.size(); i++) {
			if(!triangles[i].deleted) {
				lim::n_simp::Triangle& t=triangles[i];
				triangles[dst++]=t;
				for(int j=0; j<3; j++) {
					vertices[t.v[j]].tcount=1;
				}
			}
		}
		triangles.resize(dst);
		dst=0;
		for(int i=0; i<vertices.size(); i++) {
			if(vertices[i].tcount) {
				vertices[i].tstart=dst;
				vertices[dst].p=vertices[i].p;
				dst++;
			}
		}
		for(int i=0; i<triangles.size(); i++) {
			lim::n_simp::Triangle& t=triangles[i];
			for(int j=0; j<3; j++) {
				t.v[j]=vertices[t.v[j]].tstart;
			}
		}
		vertices.resize(dst);
	}
}
namespace lim {
	//
	// Main simplification function 
	//
	// target_count  : target nr. of triangles
	// agressiveness : sharpness to increase the threshold.
	//                 5..8 are good numbers
	//                 more iterations yield higher quality
	//
	void simplifyMesh(Mesh* mesh, int target_count, int agressiveness=7) {
		double timeStart = glfwGetTime();
		updateGlobal(mesh);
		for(n_simp::Triangle& t : triangles) {
			t.deleted=0;
		}

		// main iteration loop 

		int deleted_triangles=0; 
		std::vector<int> deleted0,deleted1;
		int triangle_count=triangles.size();
		
		for(int iter=0; iter<100; iter++) {
			// target number of triangles reached ? Then break
			printf("iteration %d - triangles %d\n",iter,triangle_count-deleted_triangles);
			if(triangle_count-deleted_triangles<=target_count)break;

			// update mesh once in a while
			if(iter%5==0) {
				update_mesh(iter);
			}

			// clear dirty flag
			for(int i=0; i<triangles.size(); i++) {
				triangles[i].dirty=0;
			}
			
			//
			// All triangles with edges below the threshold will be removed
			//
			// The following numbers works well for most models.
			// If it does not, try to adjust the 3 parameters
			//
			double threshold = 0.000000001*pow(double(iter+3),agressiveness);

			// remove vertices & mark deleted triangles			
			for(int i=0; i<triangles.size(); i++) {				
				lim::n_simp::Triangle &t=triangles[i];
				if(t.err[3]>threshold) continue;
				if(t.deleted) continue;
				if(t.dirty) continue;
				
				for(int j=0; j<3; j++){
					if(t.err[j]<threshold) 
					{
						int i0=t.v[j]; 
						n_simp::Vertex &v0 = vertices[i0]; 

						int i1=t.v[(j+1)%3]; 
						n_simp::Vertex &v1 = vertices[i1];

						// Border check
						if(v0.border != v1.border)  continue;

						// Compute vertex to collapse to
						glm::vec3 p;
						calculate_error(i0,i1,p);

						deleted0.resize(v0.tcount); // normals temporarily
						deleted1.resize(v1.tcount); // normals temporarily

						// don't remove if flipped
						if( flipped(p,i0,i1,v0,v1,deleted0) ) continue;
						if( flipped(p,i1,i0,v1,v0,deleted1) ) continue;

						// not flipped, so remove edge												
						v0.p=p;
						v0.q=v1.q+v0.q;
						int tstart=refs.size();

						update_triangles(i0,v0,deleted0,deleted_triangles);
						update_triangles(i0,v1,deleted1,deleted_triangles);
						
						int tcount=refs.size()-tstart;
				
						if(tcount<=v0.tcount)
						{
							const int ref_size = sizeof(lim::n_simp::Ref);
							// save ram
							if(tcount)memcpy(&refs[v0.tstart] ,&refs[tstart], tcount*ref_size);
						}
						else {
							// append
							v0.tstart=tstart;
						}
						v0.tcount=tcount;
						break;
					}
				}
				// done?
				if(triangle_count-deleted_triangles<=target_count)break;
			}
		}

		// clean up mesh
		compact_mesh();

		// ready
		double timeEnd=glfwGetTime();
		printf("mesh simplified - %d/%d %d%% removed in %f s\n",
			triangle_count-deleted_triangles, triangle_count,
			deleted_triangles*100/triangle_count,
			timeEnd-timeStart);
	}

	void simplifyModel(Model* model) {
		printf("\nsimplify model : %s, %d vertices\n", model->name.c_str(), model->getVerticesNum());
		for(Mesh* mesh : model->meshes) {
			simplifyMesh(mesh,2000);
		}
	}
}
#endif
