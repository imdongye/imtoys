//
//  for test pbr
//	edit learnopengl code 
//	2022-11-28 / im dong ye
//
//	TODO list:
//	1. render sphere isVertexArray가 무거운지 확인, vbo, gpu 버퍼가 언제 clear되는지 질문
//

#ifndef APP_PBR_H
#define APP_PBR_H

namespace lim
{
	class AppPbr: public AppBase
	{
	public:
        inline static constexpr const char *APP_DIR = "impbr/";
		inline static constexpr const char *APP_NAME = "impbr";
		inline static constexpr const char *APP_DISC = "ggx beckman";
	private:
		Camera *camera;
		Program *prog;
        Viewport *viewport;
		Model *model;

		int nr_rows    = 4;
		int nr_cols = 4;
		float spacing = 2.5;

		/* lights */
		bool movedLight = false;
		glm::vec3 light_position = glm::vec3(-10.0f, 10.0f, 10.0f);
		glm::vec3 light_color = glm::vec3(300.0f, 300.0f, 300.0f);

		glm::vec3 albedo = glm::vec3(0.5, 0.f, 0.f);
		bool start_dragging = false;
		std::vector<glm::vec3> metal_colors;
		float roughness = 0.3f;
		float metallic = 1.0f;
		float beckmannGamma = 1.265f;

	public:
		AppPbr(): AppBase(1680, 1120, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);

			metal_colors.push_back( {1.000, 0.782, 0.344} ); // Gold
			metal_colors.push_back( {0.664, 0.824, 0.850} ); // Zinc
			metal_colors.push_back( {0.972, 0.960, 0.915} ); // Silver
			metal_colors.push_back( {0.955, 0.638, 0.583} ); // Corper

			camera = new Camera( glm::vec3(0, 0, 12), scr_width/(float)scr_height );
			camera->fovy = 45.f;
			camera->updateProjMat();

			prog = new Program("pbr", APP_DIR);
			prog->attatch("1.1.pbr.vs").attatch("1.1.pbr.fs").link();
            
            viewport = new Viewport(new MsFramebuffer());
            viewport->framebuffer->clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

			model = ModelLoader::loadFile("common/archive/meshes/happy.obj", true);
			model->position = glm::vec3(15, 0, 0);
			model->scale = glm::vec3(50);
			model->updateModelMat();

            
            
            /* initialize static shader uniforms before rendering */
			GLuint pid = prog->use();
            setUniform(pid, "ao", 1.0f);
		}
		~AppPbr()
		{
			delete camera;
			delete prog;
			delete viewport;
			delete model;
		}
	private:
		virtual void update() final
		{
            processInput(window);
                        
			/* render to fbo in viewport */
            viewport->framebuffer->bind();

			GLuint pid = prog->use();
			setUniform(pid, "beckmannGamma", beckmannGamma);
            
            camera->aspect = viewport->framebuffer->aspect;
            camera->updateProjMat();
			setUniform(pid, "view", camera->view_mat);
			setUniform(pid, "camPos", camera->position);
            setUniform(pid, "projection", camera->proj_mat);

			/* draw light */
			glm::vec3 newPos = light_position + glm::vec3(sin(glfwGetTime() * 2.0) * 5.0, 0.0, 0.0);
			if(!movedLight) newPos = light_position;
			setUniform(pid, "lightPosition", newPos);
			setUniform(pid, "lightColor", light_color);

			glm::mat4 modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, newPos);
			modelMat = glm::scale(modelMat, glm::vec3(0.5f));
			setUniform(pid, "model", modelMat);
			renderSphere();

			/* draw spheres */
			glm::vec2 pivot = {(nr_cols-1)*spacing*0.5f, (nr_rows-1)*spacing*0.5f};

			for( int row = 0; row < nr_rows; ++row ) {
				//setUniform(pid, "metallic", row/(float)nr_rows);
				setUniform(pid, "isGGX", (row%2==0));

				for( int col = 0; col < nr_cols; ++col ) {
					if( row<2 ) {
                        // under 0.05 to error
                        setUniform(pid, "roughness", glm::clamp( col/(float)nr_cols, 0.05f, 1.0f));
						setUniform(pid, "albedo", albedo);
						setUniform(pid, "metallic", 0.f);
					}
					else {
                        // under 0.05 to error
                        setUniform(pid, "roughness", glm::clamp( 1/(float)nr_cols, 0.05f, 1.0f));
						setUniform(pid, "albedo", metal_colors[col]);
						setUniform(pid, "metallic", 1.f);
					}


					modelMat = glm::mat4(1.0f);
					modelMat = glm::translate(modelMat, glm::vec3( col*spacing-pivot.x, pivot.y-row*spacing, 0.0f ));
					setUniform(pid, "model", modelMat);

					renderSphere();
				}
			}

			/* draw test shpere */
			setUniform(pid, "albedo", metal_colors[0]);
			setUniform(pid, "metallic", metallic);
			setUniform(pid, "roughness", roughness);
			setUniform(pid, "isGGX", true);
			modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, glm::vec3(spacing*3, spacing, 0));
			modelMat *= glm::scale(glm::vec3(2.f));
			setUniform(pid, "model", modelMat);
			renderSphere();

			setUniform(pid, "isGGX", false);
			modelMat = glm::mat4(1.0f);
			modelMat = glm::translate(modelMat, glm::vec3(spacing*3, -spacing, 0));
			modelMat *= glm::scale(glm::vec3(2.f));
			setUniform(pid, "model", modelMat);
			renderSphere();


			setUniform(pid, "isGGX", true);
			setUniform(pid, "model", model->model_mat);
			for( Mesh* mesh: model->meshes ) {
				mesh->draw();
			}
            
            viewport->framebuffer->unbind();
            
            // clear backbuffer
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		}
		virtual void renderImGui() final
		{
			imgui_modules::ShowExampleAppDockSpace([]() {});

			ImGui::ShowDemoWindow();
            
			// draw framebuffer on viewport gui
            viewport->drawImGui();

			ImGui::Begin("state##impbr");
            ImGui::Text((viewport->dragging)?"dragging":"not dragging");
			ImGui::Checkbox("moved light:", &movedLight);
			ImGui::ColorPicker3("albedo", (float*)&albedo);
			ImGui::SliderFloat("roughness", &roughness, 0.f, 1.f);
			ImGui::SliderFloat("metallic", &metallic, 0.f, 1.f);
			ImGui::SliderFloat("beckmannGamma", &beckmannGamma, 0.7f, 2.f);
            ImGui::End();
		}
	private:
		void renderSphere()
		{
			static unsigned int sphereVAO = 0;
			static unsigned int indexCount = 0;

			if( glIsVertexArray(sphereVAO)==GL_FALSE ) {
				unsigned int vbo, ebo;

				glGenVertexArrays(1, &sphereVAO);
				glGenBuffers(1, &vbo);
				glGenBuffers(1, &ebo);

				std::vector<glm::vec3> positions;
				std::vector<glm::vec2> uv;
				std::vector<glm::vec3> normals;
				std::vector<unsigned int> indices;

				const unsigned int X_SEGMENTS = 64;
				const unsigned int Y_SEGMENTS = 64;
				const float PI = 3.14159265359;
				for( unsigned int y = 0; y <= Y_SEGMENTS; ++y ) {
					for( unsigned int x = 0; x <= X_SEGMENTS; ++x ) {
						float xSegment = (float)x / (float)X_SEGMENTS;
						float ySegment = (float)y / (float)Y_SEGMENTS;
						float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
						float yPos = std::cos(ySegment * PI);
						float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

						positions.push_back(glm::vec3(xPos, yPos, zPos));
						uv.push_back(glm::vec2(xSegment, ySegment));
						normals.push_back(glm::vec3(xPos, yPos, zPos));
					}
				}

				bool oddRow = false;
				for( unsigned int y = 0; y < Y_SEGMENTS; ++y ) {
					if( !oddRow ) // even rows: y == 0, y == 2; and so on
					{
						for( unsigned int x = 0; x <= X_SEGMENTS; ++x ) {
							indices.push_back(y       * (X_SEGMENTS + 1) + x);
							indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
						}
					} else {
						for( int x = X_SEGMENTS; x >= 0; --x ) {
							indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
							indices.push_back(y       * (X_SEGMENTS + 1) + x);
						}
					}
					oddRow = !oddRow;
				}
				indexCount = (int)indices.size();

				std::vector<float> data;
				for( unsigned int i = 0; i < positions.size(); ++i ) {
					data.push_back(positions[i].x);
					data.push_back(positions[i].y);
					data.push_back(positions[i].z);
					if( normals.size() > 0 ) {
						data.push_back(normals[i].x);
						data.push_back(normals[i].y);
						data.push_back(normals[i].z);
					}
                    if( uv.size() > 0 ) {
                        data.push_back(uv[i].x);
                        data.push_back(uv[i].y);
                    }
				}
				glBindVertexArray(sphereVAO);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
				float stride = (3 + 3 + 2) * sizeof(float);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
			}

			glEnable(GL_DEPTH_TEST);
			glBindVertexArray(sphereVAO);
			glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
		}
		void processInput(GLFWwindow *window)
		{
			static const double cameraMoveSpeed = 3.9;
			if( glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS )
				glfwSetWindowShouldClose(window, true);

			if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::FORWARD, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::BACKWARD, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::LEFT, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::RIGHT, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::UP, delta_time, cameraMoveSpeed);
			if( glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS )
				camera->move(Camera::MOVEMENT::DOWN, delta_time, cameraMoveSpeed);
			camera->updateFreeViewMat();
		}
		/* glfw callback */
		virtual void framebufferSizeCallback(double width, double height) final
		{
			glViewport(0, 0, width, height);
		}
		virtual void scrollCallback(double xoff, double yoff) final
		{
			if( !viewport->focused ) return;
			camera->shiftZoom(yoff*5.f);
			camera->updateProjMat();
		}
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
		}
		virtual void mouseBtnCallback(int button, int action, int mods) final
		{
			start_dragging = action==GLFW_PRESS;
		}
		virtual void cursorPosCallback(double xpos, double ypos) final
		{
			static const float rotateSpeed = 0.08f;
			static float xoff, yoff;
			static double xold, yold;
			
			if( !viewport->dragging )return;

			if( start_dragging ) {
				xold = xpos;
				yold = ypos;
                start_dragging = false;
			}

			xoff = xpos - xold;
			yoff = yold - ypos;

			xold = xpos;
			yold = ypos;

			camera->rotateCamera(xoff * rotateSpeed, yoff * rotateSpeed);
			camera->updateFreeViewMat();
        }
	};
}

#endif
