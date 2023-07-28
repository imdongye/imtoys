//
//  framework template
//	2022-11-14 / im dong ye
//
//	TODO list:
//	1. define var
//	2. class name
//  3. dir, name, disc
//

#ifndef APP_TEMP_H
#define APP_TEMP_H

#include <limbrary/limclude.h>
#include <Eigen/Eigen>
#include "icp.h"

namespace lim
{
	class AppICP: public AppBase
	{
	public:
		inline static constexpr const char const *APP_NAME = "test icp";
		inline static constexpr const char const *APP_DIR = "imtests/";
		inline static constexpr const char const *APP_DISC = "hello, world";
	private:
		AutoCamera *camera;
		Program *prog;
		Viewport *viewport;
		Model *model;
		Mesh *src, *dest;
		glm::mat4 icpMat = glm::mat4(1.0);

	public:
		AppICP(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);

			prog = new Program("pbr", APP_DIR);
			prog->attatch("debug.vs").attatch("debug.fs").link();

			viewport = new Viewport(new MsFramebuffer());
			viewport->framebuffer->clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
			camera = new AutoCamera(window, viewport, 0, {0,1,5}, {0,1,0});


			model = ModelLoader::loadFile("common/archive/meshes/woody.obj", true);


			dest = copyTranfromedMesh(*model->meshes[0], model->model_mat);
			dest->draw_mode = GL_POINTS;
			dest->color = {1,1,1};

			glm::mat4 rigidMat = glm::translate(glm::vec3 {3,1, 0}) * glm::rotate(0.2f, glm::vec3 {0,0,1});

			src = copyTranfromedMesh(*dest, rigidMat); // todo : simplify 
			src->draw_mode = GL_POINTS;
			src->color = {0.6,1,0.7};
			

			delete model; model = nullptr;

			/* initialize static shader uniforms before rendering */
			GLuint pid = prog->use();
			setUniform(pid, "ao", 1.0f);
		}
		~AppICP()
		{
			delete camera;
			delete prog;
			delete viewport;
			delete model;
		}
	private:
		void update() override
		{
			viewport->framebuffer->bind();

			GLuint pid = prog->use();

			setUniform(pid, "viewMat", camera->view_mat);
			setUniform(pid, "cameraPos", camera->position);
			setUniform(pid, "projMat", camera->proj_mat);

			setUniform(pid, "modelMat", glm::mat4(1));
			dest->draw(pid);

			setUniform(pid, "modelMat", icpMat);
			src->draw(pid);
			//model->meshes[0]->draw(pid);
			 
			viewport->framebuffer->unbind();

			// clear backbuffer
			glEnable(GL_DEPTH_TEST);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, fb_width, fb_height);
			glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		void renderImGui() override
		{
			ImGui::DockSpaceOverViewport();

			viewport->drawImGui();

			static bool isoverlay = true;
			ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
			ImGui::Begin("icp tester", &isoverlay, window_flags);
			if( ImGui::Button("iterate") ) {
				const int nr_verts = src->vertices.size();
				// 열벡터로 나열
				Eigen::MatrixXd srcdata = Eigen::MatrixXd::Ones(3, nr_verts);
				Eigen::MatrixXd dstdata = Eigen::MatrixXd::Ones(3, nr_verts);

				for( int i=0; i<nr_verts; i++ ) {
					glm::vec3 p = src->vertices[i].p;
					srcdata.block<3,1>(0, i) << p.x, p.y, p.z;

					p = dest->vertices[i].p;
					dstdata.block<3,1>(0, i) << p.x, p.y, p.z;
				}

				Eigen::Matrix4d eMat = icp(srcdata, dstdata);

				for( int i=0; i<4; i++ ) for( int j=0; j<4; j++ ) {
					icpMat[i][j] = eMat(j, i);
				}
				printf("done\n");
			}
			ImGui::End();
		}
	private:
		Mesh* copyTranfromedMesh(const Mesh& ori, const glm::mat4& modelMat)
		{
			Mesh* rst = new Mesh();
			rst->indices = ori.indices;

			for( const n_mesh::Vertex& v : ori.vertices ) {
				n_mesh::Vertex newV = v;
				glm::vec4 hc = {v.p.x, v.p.y, v.p.z, 1};
				newV.p = glm::vec3(modelMat*hc);

				hc = {v.n.x, v.n.y, v.n.z, 0};
				newV.n = glm::vec3(modelMat*hc);

				newV.uv = v.uv;
				
				rst->vertices.push_back(newV);
			}

			rst->has_texture = false;
			rst->setupMesh();

			return rst;
		}
	};
}

#endif
