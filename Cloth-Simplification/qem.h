#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <map>
#include <vector>
#include <string>
#include <math.h>
#include <float.h> //FLT_EPSILON, DBL_EPSILON
#include <glm/glm.hpp>
#include <Eigen/Core>

#include "lim/model.h"
#include <GLFW/glfw3.h>

namespace qem
{
	struct Triangle { int v[3]; double err[4]; };
	struct Vertex { glm::vec3 p; Eigen::Matrix4f Q; };
	struct Edge { Vertex v1; Vertex v2; };

	std::vector<Vertex> vertices;
	std::vector<Triangle> triangles;
	std::vector<Edge> edges;

	int triangles_initSize = 0;

	void updateGlobal(lim::Mesh* mesh) {
		vertices.clear();
		triangles.clear();
		edges.clear();

		for (lim::n_model::Vertex& v : mesh->vertices)
		{
			Vertex temp;
			temp.p.x = v.p.x;
			temp.p.y = v.p.y;
			temp.p.z = v.p.z;
			vertices.push_back(temp);
		}

		// make triangles & edge
		triangles.resize(mesh->indices.size() / 3);
		int triCount = 0;
		int vertCount = 0;
		for (GLuint idx : mesh->indices)
		{
			triangles[triCount].v[vertCount++] = idx;
			//fqms::triangles[triCount].uvs
			if (vertCount == 3)
			{
				vertCount = 0;
				triCount++;
			}
		}
		if (triCount * 3 != mesh->indices.size()
			|| mesh->indices.size() % 3 != 0)
		{
			fprintf(stderr, "simplify failed : mesh is not triangle mesh\n");
		}

		edges.resize(vertices.size());
	}

	void mesh_simplification(int target_count, double agressiveness = 7, bool verbose = false)
	{
		// Initialize Q matrices
		for (int i = 0; i < vertices.size(); i++)
			vertices[i].Q = Eigen::Matrix4f::Zero();

		// Compute Q matrices
		for (int i = 0; i < triangles.size(); i++)
		{
			int vid0 = triangles[i].v[0];
			int vid1 = triangles[i].v[1];
			int vid2 = triangles[i].v[2];

			// Plane equation
			glm::vec3 p1, p2, p3;
			p1 = vertices[vid0].p;
			p2 = vertices[vid1].p;
			p3 = vertices[vid2].p;

			Eigen::Vector4f p;
			p[0] = p1.y * (p2.z - p3.z) + p2.y * (p3.z - p1.z) + p3.y * (p1.z - p2.z);
			p[1] = p1.z * (p2.x - p3.x) + p2.z * (p3.x - p1.x) + p3.z * (p1.x - p2.x);
			p[2] = p1.x * (p2.y - p3.y) + p2.x * (p3.y - p1.y) + p3.x * (p1.y - p2.y);
			p[3] = - (p1.x * (p2.y * p3.z - p3.y * p2.z) + p2.x * (p3.y * p1.z - p1.y * p3.z) + p3.x * (p1.y * p2.z - p2.y * p1.z));

			float normalize = sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);

			p /= normalize;

			Eigen::Matrix4f K = p * p.transpose();

			vertices[vid0].Q += K;
			vertices[vid1].Q += K;
			vertices[vid2].Q += K;

			//Select all valid pairs
			Edge edge;
			if (vid0 < vid2) { edge.v1 =  }
		}							   

	}


	void updateMesh(lim::Mesh* mesh)
	{
		mesh->vertices.clear();
		mesh->indices.clear();

		for (Vertex& v : vertices)
		{
			lim::n_model::Vertex temp;
			temp.p.x = v.p.x;
			temp.p.y = v.p.y;
			temp.p.z = v.p.z;
			mesh->vertices.push_back(temp);
		}

		// make triangles & edge
		mesh->indices.resize(triangles.size() * 3);

		int idxCount = 0;
		for (Triangle& tri : triangles)
		{
			for (int i = 0; i < 3; i++)
			{
				mesh->indices[idxCount++] = tri.v[i];
			}
		}
		mesh->setupMesh();
	}

	lim::Mesh* meshSimplification(lim::Mesh* mesh, float lived_pct, int agressiveness = 7)
	{
		if (lived_pct > 1 || lived_pct < 0)
		{
			printf("error : simplify percent range error");
			return nullptr;
		}
		updateGlobal(mesh);

		printf("\nsimplify mesh : %s, %d tris\n", mesh->name.c_str(), triangles.size());

		int target_count = (int)(triangles.size() * lived_pct);
		double start = glfwGetTime();
		mesh_simplification(target_count, 7.0, true);

		printf("\nsimplify mesh : %s, %d tris, %lfsec\n", mesh->name.c_str(), triangles.size(), glfwGetTime() - start);

		lim::Mesh* result = new lim::Mesh(mesh->name.c_str());
		updateMesh(result);
		return result;
	}

	lim::Model* modelSimplification(lim::Model* model, int i, float lived_pct = 0.8f) {
		printf("\nsimplify model : %s, %d vertices\n", model->name.c_str(), model->verticesNum);
		std::vector<lim::Mesh*> new_meshes;
		for (lim::Mesh* mesh : model->meshes)
		{
			lim::Mesh* simp_mesh = meshSimplification(mesh, lived_pct);
			if (simp_mesh != nullptr)
				new_meshes.push_back(simp_mesh);
		}
		lim::Model* result = new lim::Model(new_meshes, model->textures_loaded, model->name);
		return result;
	}
};