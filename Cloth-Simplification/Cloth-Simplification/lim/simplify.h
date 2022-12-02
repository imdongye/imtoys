//
//	2022-08-07 / im dong ye
//
//	TODO list:
//  1. fix with half edge data structure
//
//

#ifndef SIMPLIFY_H
#define SIMPLIFY_H
#include <Eigen/core>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <glm/gtc/matrix_access.hpp>


namespace simp
{
	class SymetricMatrix {
	public:
		// 0 1 2 3
		//   4 5 6
		//     7 8
		//       9
		double m[10];
	public:
		SymetricMatrix(double c=0) {
			for(int i=0; i<10; i++) {
				m[i] = c;
			}
		}
		SymetricMatrix( double m11, double m12, double m13, double m14,
									double m22, double m23, double m24,
												double m33, double m34,
															double m44 ) {
			m[0]=m11; m[1]=m12; m[2]=m13; m[3]=m14;
					  m[4]=m22; m[5]=m23; m[6]=m24;
								m[7]=m33; m[8]=m34;
										  m[9]=m44;
		}
		// outer product
		SymetricMatrix(double a, double b, double c, double d) {
			m[0]=a*a; m[1]=a*b; m[2]=a*c; m[3]=a*d;
					  m[4]=b*b; m[5]=b*c; m[6]=b*d;
								m[7]=c*c; m[8]=c*d;
										  m[9]=d*d;
		}
		SymetricMatrix(glm::vec4 v) : SymetricMatrix(v.x, v.y, v.z, v.w) {
		}

		// 역행렬 결과값 구할때 사용
		// 역행렬 : adj/det
		double det( int a11, int a12, int a13, 
					int a21, int a22, int a23,
					int a31, int a32, int a33 ) {
			// 연장 대각선 규칙
			double det =  m[a11]*m[a22]*m[a33] + m[a12]*m[a23]*m[a31] + m[a13]*m[a21]*m[a32]
						- m[a13]*m[a22]*m[a31] - m[a11]*m[a23]*m[a32] - m[a12]*m[a21]*m[a33];
			return det;
		}
		double operator[](int c) const { return m[c]; }
		const SymetricMatrix operator+(const SymetricMatrix& n) const {
			return SymetricMatrix(m[0]+n[0], m[1]+n[1], m[2]+n[2], m[3]+n[3],
											 m[4]+n[4], m[5]+n[5], m[6]+n[6],
														m[7]+n[7], m[8]+n[8],
																   m[9]+n[9]);
		}
		SymetricMatrix& operator+=(const SymetricMatrix& n) {
			m[0]+=n[0]; m[1]+=n[1]; m[2]+=n[2]; m[3]+=n[3];
						m[4]+=n[4]; m[5]+=n[5]; m[6]+=n[6];
									m[7]+=n[7]; m[8]+-n[8];
												m[9]+=n[9];
		}
	};

	const GLuint OOI_uint = ~GLuint(0);

	typedef lim::n_model::Vertex M_Vertex;

	struct HalfEdge {
		GLuint twin, next, prev; // half edge idx
		GLuint vi, ti; // vert idx, tri idx
		bool operator==(const HalfEdge& he) const {
			return twin==he.twin&&next==he.next&&prev==he.prev;
		}
		bool operator!=(const HalfEdge& he) const {
			return !(*this==he);
		}
	};
	struct Triangle {
		GLuint hei; // half edge idx
		glm::vec4 plane;
		bool removed;
	};
	struct Vertex : M_Vertex {
		GLuint hei;
		SymetricMatrix qMat;
		void pull(const M_Vertex& copy) {
			(M_Vertex)(*this) = copy;
		}
	};

	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;
	std::vector<HalfEdge> halfedges;
	/*
	void updatePlane(Triangle& triangle) 
	{
		HalfEdge he = halfedges[triangle.hei];
		glm::vec3 ps[3];
		for(int i=0; i<3; i++) {
			ps[i] = vertices[he.vi].p;
			he = halfedges[he.next];
		}
		glm::vec3 v0 = ps[1]-ps[0];
		glm::vec3 v1 = ps[2]-ps[0];
		glm::vec3 n = glm::normalize(glm::cross(v0, v1));
		float d = glm::dot(ps[0], n);
		triangle.plane = glm::vec4(n,-d);
	}
	void updateVertexError(int vertexIndex)
	{
		SymetricMatrix qMat(0);
		HalfEdge he = halfedges[vertices[vertexIndex].hei];
		do {
			Triangle& tri = triangles[he.ti];
			qMat += SymetricMatrix(tri.plane); // outer product
			he = halfedges[halfedges[he.prev].next];
		}
		std::vector<GLuint>& adjTris = vertAdjTris[vertexIndex];

		for( GLuint adjTri : adjTris ) {
			Triangle& tri = triangles[adjTri];
			qMat += glm::outerProduct(tri.plane, tri.plane);
		}
		vertQs[vertexIndex] = qMat;
	}
	void updateEdgeError(Edge e)
	{
		lim::n_model::Vertex newVert = vertices[e.v[0]];
		e.edgeQ = vertQs[e.v[0]] + vertQs[e.v[1]];

		// new position //
		//vec4 np = vec4((vertices[e.v[0]].p + vertices[e.v[1]].p) / 2.0f, 1.0);
		glm::mat4 dtA = e.edgeQ;
		dtA = glm::row(dtA,3, glm::vec4(0,0,0,1)); 
		// dtA * np = vec4(0,0,0,1)
		glm::vec4 np = glm::inverse(dtA)*glm::vec4(0,0,0,1);

		// new normal //
		glm::vec3 nn = (vertices[e.v[0]].n + vertices[e.v[1]].n) / 2.0f;

		// new uv //
		glm::vec2 nuv = (vertices[e.v[0]].uv + vertices[e.v[1]].uv) / 2.0f;

		newVert.p = np; newVert.n = nn; newVert.uv = nuv;

		// temp is a row vector // vt*Q*v
		glm::vec4 temp = e.edgeQ * np;
		e.edgeError = glm::dot(np, temp);
	}

	void updateGlobal(const lim::Mesh* mesh) {
		vertices.clear();
		triangles.clear();
		edges.clear();
		vertAdjTris.clear();
		vertQs.clear();

		// deep copy vertices
		vertices = mesh->vertices;

		// make triangles & edge
		triangles.resize(mesh->indices.size()/3);
		vertAdjTris.resize(vertices.size());
		int triCount=0;
		int vertCount=0;
		for(GLuint idx: mesh->indices) {
			vertAdjTris[idx].push_back(triCount);
			triangles[triCount].v[vertCount++] = idx;
			triangles[triCount].removed = false;
			if(vertCount == 3) {
				Triangle& t = triangles[triCount];
				updatePlane(t);

				Edge e0(t.v[0], t.v[1]);
				Edge e1(t.v[1], t.v[2]);
				Edge e2(t.v[2], t.v[0]);

				e0.adjTri[0] = e1.adjTri[0] = e2.adjTri[0] = triCount;

				edges.push_back(e0);
				edges.push_back(e1);
				edges.push_back(e2);

				vertCount = 0;
				triCount++;
			}
		}
		if( triCount*3!=mesh->indices.size() 
			|| mesh->indices.size()%3!=0 ) {
			fprintf(stderr, "simplify failed : mesh is not triangle mesh\n");
		}

		// already maked duplicated edges in "make triangles"
		// we just remove duplicates;
		// O(N^2) + deep copy
        std::vector<Edge> nodup;
        for(GLuint i=0; i<edges.size(); i++) {
            bool found = false;
            for(GLuint j=i+1; j<edges.size(); j++) {
                if(edges[i] == edges[j]) {
                    found = true;
                    edges[j].adjTri[1] = edges[i].adjTri[0];
                    break;
                }
            }
            if(found == false) {
                nodup.push_back(edges[i]);
            }
        }
        edges = nodup;
        
		// update errors
		vertQs.resize(vertices.size());
		for (int i=0; i<vertices.size(); i++) {
			updateVertexError(i);
		}
		for (int i=0; i<edges.size(); i++) {
			updateEdgeError(edges[i]);
		}
	}
	void collapseEdge() {
		// 오름차순으로 정렬후 에러가 적은것을 합친다.
		make_heap(edges.begin(), edges.end(), greaterEdgeErr());
		pop_heap(edges.begin(), edges.end());
		const Edge& r_edge = edges.back();
		const GLuint n_idx = r_edge.v[0]; // new vert idx
		const GLuint d_idx = r_edge.v[1]; // delete vert idx


		vertices[n_idx] = r_edge.newVert;
		vertQs[n_idx] = r_edge.edgeQ;

		// consider non-maniford
		// 바운더리면 연결삼각형이 한개이고
		// 공통 버텍스도 한개다.
		std::vector<GLuint> rmTris;
		std::vector<Edge> rmEdges;
		std::vector<GLuint> uniVerts;
		if(r_edge.adjTri[0] != OOI_uint)
			rmTris.push_back(r_edge.adjTri[0]);
		if(r_edge.adjTri[1] != OOI_uint)
			rmTris.push_back(r_edge.adjTri[1]);

        for(GLuint rmTri : rmTris) {
			Triangle& tri= triangles[rmTri];
			tri.removed = true;

			for(int i=0; i<3; i++) {
				if(tri.v[i]!=n_idx&&tri.v[i]!=d_idx) {
					uniVerts.push_back(tri.v[i]);
					break;
				}
			}
            GLuint uniVert = uniVerts.back();
            Edge* n_edge = nullptr;
			Edge* d_edge = nullptr;
            for(Edge& e : edges) {
                if(e == Edge(n_idx,uniVert))
                    n_edge = &e;
                else if(e == Edge(d_idx,uniVert))
                    d_edge = &e;
            }
			if(n_edge != nullptr && d_edge != nullptr) {
				n_edge->adjTri[(n_edge->adjTri[0]==rmTri)?0:1] = d_edge->adjTri[(n_edge->adjTri[0]==rmTri)?1:0];
				edges.erase(std::find(edges.begin(), edges.end(), *d_edge));
			}
			else if(n_edge != nullptr)
				n_edge->adjTri[(n_edge->adjTri[0]==rmTri)?0:1] = OOI_uint;
		}
		// update triangle
		for(GLuint editTriIdx : vertAdjTris[r_edge.v[1]]) {
			Triangle& tri = triangles[editTriIdx];
			if(tri.removed)
				continue;
			for(int i=0; i<3; i++) {
				// per idx
				if(tri.v[i] == d_idx) {
					tri.v[i] = n_idx;
				}
			}
			vertAdjTris[r_edge.v[0]].push_back(editTriIdx);
		}

        for(Edge& e : edges) {
            if(e.isIn(n_idx)) {
                updateEdgeError(e);
            }
        }

		vertAdjTris[r_edge.v[1]].clear();

		edges.pop_back();
	}

	void updateVertsAndTris(lim::Mesh* mesh) {
		GLuint oriVertsSize = vertAdjTris.size(); // vertices.size()
		std::vector<int> idxadd(oriVertsSize);
		mesh->vertices.clear();
		idxadd[0] = 0;
		mesh->vertices.push_back(vertices[0]);

		for(int i=1; i<oriVertsSize; i++) {
			if(vertAdjTris[i].size() > 0) {
				idxadd[i] = idxadd[i-1];
				mesh->vertices.push_back(vertices[i]);
			}
			else {
				idxadd[i] = idxadd[i-1]-1;
			}
		}
		mesh->indices.clear();
		for(Triangle& tri : triangles) {
			if(tri.removed)
				continue;

			for(int j=0; j<3; j++) {
				int newIdx = tri.v[j] + idxadd[tri.v[j]];
				mesh->indices.push_back(newIdx);
			}
		}
	}
	*/

	

	void updateGlobal(lim::Mesh* mesh) {
		vertices.clear();
		triangles.clear();
		halfedges.clear();

		vertices.reserve(mesh->vertices.size());
		for(lim::n_model::Vertex& v : mesh->vertices) {
			Vertex temp;
			temp.pull(v);
			vertices.push_back(temp);
		}
		/*
		// make triangles & edge
		triangles.resize(mesh->indices.size()/3);
		int triCount=0;
		int vertCount=0;
		std::vector<GLuint>
		for(GLuint idx: mesh->indices) {
			HalfEdge he;
			he.vi = idx;
			if(vertCount == 3) {
				vertCount = 0;
				triCount++;
			}
		}
		if( triCount*3!=mesh->indices.size() 
			|| mesh->indices.size()%3!=0 ) {
			fprintf(stderr, "simplify failed : mesh is not triangle mesh\n");
		}*/
	}
	/*
	void updateMesh(lim::Mesh* mesh) {
		mesh->vertices.clear();
		mesh->indices.clear();

		for(Vertex& v : vertices) {
			lim::n_model::Vertex temp;
			temp.p.x = v.p.x;
			temp.p.y = v.p.y;
			temp.p.z = v.p.z;
			mesh->vertices.push_back(temp);
		}

		// make triangles & edge
		mesh->indices.resize(triangles.size()*3);

		int idxCount=0;
		for(Triangle& tri: triangles) {
			for(int i=0; i<3; i++) {
				mesh->indices[idxCount++] = tri.v[i];
			}
		}
	}
	*/
}


namespace lim {
	void simplifyMesh(Mesh* mesh, float lived_pct, int agressiveness=7) {
		if(lived_pct>1||lived_pct<0)
			return;
		
		//printf("simplify mesh %d triangles, %d vertices\n", triangles.size(), vertices.size());

		const GLuint target_nr_tris = triangles.size() * lived_pct;
		GLuint cur_nr_tris = triangles.size();

		while(cur_nr_tris > target_nr_tris) {
			
			//collapseEdge();

			cur_nr_tris-=2;// 바운더리면 하나일수있음 수정해야함
		}
		//updateVertsAndTris(mesh);

		printf("mesh simplified: %d triangles, %d vertices\n"
			, mesh->indices.size()/3, mesh->vertices.size());
	}

	void simplifyModel(Model* model, float lived_pct = 0.8f) {
		printf("\nsimplify model : %s, %d vertices\n", model->name.c_str(), model->getVerticesNum());
		for(Mesh* mesh : model->meshes) {
			simplifyMesh(mesh,lived_pct);
		}
	}
}
#endif