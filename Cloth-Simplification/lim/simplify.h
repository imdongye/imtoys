#ifndef SIMPLIFY_H
#define SIMPLIFY_H
#include <Eigen/core>
#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <glm/gtc/matrix_access.hpp>


namespace 
{
	const GLuint OOE_uint = ~GLuint(0);
	struct Edge {
		GLuint v[2];
		GLuint adjTri[2];

		float edgeError = 0;
		glm::mat4 edgeQ;
		lim::n_model::Vertex newVert;

		Edge(GLuint v0, GLuint v1) {
			if(v0<v1) {
				v[0] = v0;
				v[1] = v1;
			} else {
				v[0] = v0;
				v[1] = v1;
			}
			adjTri[0] = adjTri[1] = OOE_uint;
		}
		bool operator==(const Edge& other) const {
			return (v[0] == other.v[0] && v[1] == other.v[1])
				|| (v[0] == other.v[1] && v[1] == other.v[0]);
		}
		bool operator!=(const Edge& other) const {
			return !(*this==other);
		}
		bool operator < (const Edge& other) const {
			if (v[0] < other.v[0])
				return true;
			else if(v[0] == other.v[0])
				return (v[1] < other.v[1]);
			else
				return false;
		}
		GLuint getOppo(GLuint vidx) {
			return (v[0]==vidx) ? v[1] : ((v[1]==vidx) ? v[0] : OOE_uint);
		}
		bool isIn(GLuint vidx) {
			return v[0]==vidx||v[1]==vidx;
		}
		bool isIn(std::vector<GLuint> vidxs) {
			for(GLuint vidx : vidxs) {
				if(v[0]==vidx||v[1]==vidx)
					return true;
			}
			return false;
		}
	};
	struct Triangle {
		GLuint v[3]; // index
		glm::vec4 plane; // n, -d
		Edge* e[3];
		bool removed;
	};

	// for std::make_heap();
	// like: std::make_heap(edges.begin(), edges.end(), greaterEdgeErr());
	struct greaterEdgeErr
	{
		bool operator()(const Edge* e0, const Edge* e1) const
		{
			return e0->edgeError > e0->edgeError;
		}
	};

	std::vector<lim::n_model::Vertex> vertices;
	std::vector<Triangle> triangles;
	std::vector<Edge*> edges;
	std::vector<std::vector<GLuint>> vertAdjTris;
	std::vector<glm::mat4> vertQs; // per vertex
	
	void updatePlane(Triangle& triangle) 
	{
		glm::vec3 v0 = vertices[triangle.v[1]].p-vertices[triangle.v[0]].p;
		glm::vec3 v1 = vertices[triangle.v[2]].p-vertices[triangle.v[0]].p;
		glm::vec3 n = glm::normalize(glm::cross(v0, v1));
		float d = glm::dot(vertices[triangle.v[0]].p, n);
		triangle.plane = glm::vec4(n,-d);
	}
	void updateVertexError(int vertexIndex)
	{
		glm::mat4 qMat = glm::mat4(0);
		std::vector<GLuint>& adjTris = vertAdjTris[vertexIndex];

		for( GLuint adjTri : adjTris ) {
			Triangle& tri = triangles[adjTri];
			qMat += glm::outerProduct(tri.plane, tri.plane);
		}
		vertQs[vertexIndex] = qMat;
	}
	void updateEdgeError(Edge* e)
	{
		lim::n_model::Vertex newVert = vertices[e->v[0]];
		e->edgeQ = vertQs[e->v[0]] + vertQs[e->v[1]];

		// new position //
		//vec4 np = vec4((vertices[e.v[0]].p + vertices[e.v[1]].p) / 2.0f, 1.0);
		glm::mat4 dtA = e->edgeQ;
		dtA = glm::row(dtA,3, glm::vec4(0,0,0,1)); 
		// dtA * np = vec4(0,0,0,1)
		glm::vec4 np = glm::inverse(dtA)*glm::vec4(0,0,0,1);

		// new normal //
		glm::vec3 nn = (vertices[e->v[0]].n + vertices[e->v[1]].n) / 2.0f;

		// new uv //
		glm::vec2 nuv = (vertices[e->v[0]].uv + vertices[e->v[1]].uv) / 2.0f;

		newVert.p = np; newVert.n = nn; newVert.uv = nuv;

		// temp is a row vector // vt*Q*v
		glm::vec4 temp = e->edgeQ * np; 
		e->edgeError = glm::dot(np, temp);
	}
	void removeDups() {
		std::vector<Edge*> nodup;
		for(GLuint i=0; i<edges.size(); i++) {
			bool found = false;
			for(GLuint j=i+1; j<edges.size(); j++) {
				if(*edges[i] == *edges[j]) {
					found = true;
					edges[j]->adjTri[1] = edges[i]->adjTri[0];
					break;
				}
			}
			if(found == false) {
				nodup.push_back(edges[i]);
			}
		}
		edges = nodup;
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

				Edge* e0 = new Edge(t.v[0], t.v[1]);
				Edge* e1 = new Edge(t.v[1], t.v[2]);
				Edge* e2 = new Edge(t.v[2], t.v[0]);

				e0->adjTri[0] = e1->adjTri[0] = e2->adjTri[0] = triCount;

				edges.push_back(e0);
				t.e[0] = edges.back();
				edges.push_back(e1);
				t.e[1] = edges.back();
				edges.push_back(e2);
				t.e[2] = edges.back();

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
		removeDups();

		// update errors
		vertQs.resize(vertices.size());
		for (int i=0; i<vertices.size(); i++) {
			updateVertexError(i);
		}
		for (int i=0; i<edges.size(); i++) {
			updateEdgeError(edges[i]);
		}
	}
	void edgeCollapse() {
		// 오름차순으로 정렬후 에러가 적은것을 합친다.
		make_heap(edges.begin(), edges.end(), greaterEdgeErr());
		pop_heap(edges.begin(), edges.end());
		const Edge& r_edge = *edges.back();
		const GLuint n_idx = r_edge.v[0]; // new vert idx
		const GLuint d_idx = r_edge.v[1]; // delete vert idx


		vertices[n_idx] = r_edge.newVert;
		vertQs[n_idx] = r_edge.edgeQ;

		// consider non-maniford
		// 바운더리면 연결삼각형이 한개이고
		// 공통 버텍스도 한개다.
		std::vector<GLuint> rmTris;
		std::vector<Edge*> rmEdges;
		std::vector<GLuint> uniVerts;
		if(r_edge.adjTri[0] != OOE_uint)
			rmTris.push_back(r_edge.adjTri[0]);
		if(r_edge.adjTri[1] != OOE_uint)
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

			// 합쳐지는 edge 맞은편 삼각형 연결
			const GLuint uniVert = uniVerts.back();
			GLuint oppoTri = OOE_uint;
			Edge* oppoEdge = nullptr;
			for(int i=0; i<3; i++) {
				Edge* e = tri.e[i];
				if(*e == Edge(d_idx, uniVert)) {
					oppoTri = e->adjTri[(e->adjTri[0]==rmTri)?1:0];
					rmEdges.push_back(e);
					break;
				}
			}
			for(int i=0; i<3; i++) {
				Edge* e = tri.e[i];
				if(*e == Edge(n_idx, uniVert)) {
					e->adjTri[(e->adjTri[0]==rmTri)?0:1] = oppoTri;
					oppoEdge = e;
					break;
				}
			}
			// 삼각형 edge 복구
			if(oppoTri!=OOE_uint) {
				Triangle& et = triangles[oppoTri];
				for(int i=0; i<3; i++) {
					if(*et.e[i]==Edge(d_idx, uniVert)) {
						et.e[i] = oppoEdge;
						break;
					}
				}
			}
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
				// per edge
				Edge* t_edge = tri.e[i];
				if(t_edge->v[0] == d_idx)
					t_edge->v[0] = n_idx;
				else if(t_edge->v[1] == d_idx)
					t_edge->v[1] = n_idx;
			}
			vertAdjTris[r_edge.v[0]].push_back(editTriIdx);
		}

		// get new vert adj edges
		std::set<Edge*> adjEdges;
		for(GLuint editTriIdx : vertAdjTris[r_edge.v[0]]) {
			Triangle& tri = triangles[editTriIdx];
			if(tri.removed)
				continue;
			for(int i=0; i<3; i++) {
				Edge* t_edge = tri.e[i];
				if(t_edge->isIn(n_idx)) {
					adjEdges.insert(t_edge);
				}
			}
		}

		// update Edge Error
		for(Edge* adjEdge : adjEdges) {
			updateEdgeError(adjEdge);
		}

		vertAdjTris[r_edge.v[1]].clear();

		delete edges.back();
		edges.pop_back();

		// 합쳐지는 edge삭제
		for(Edge* rmEdge : rmEdges) {
			std::vector<Edge*>::iterator f=std::find(edges.begin(), edges.end(), rmEdge);
			delete rmEdge;
			edges.erase(f);
		}

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
}


namespace lim {
	void simplifyMesh(Mesh* mesh, float lived_pct, int agressiveness=7) {
		if(lived_pct>1||lived_pct<0)
			return;
		updateGlobal(mesh);
		
		printf("simplify mesh %d triangles, %d vertices\n", triangles.size(), vertices.size());

		const GLuint target_nr_tris = triangles.size() * lived_pct;
		GLuint cur_nr_tris = triangles.size();

		while(cur_nr_tris > target_nr_tris) {
			
			edgeCollapse();

			cur_nr_tris-=2;
		}
		updateVertsAndTris(mesh);

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
