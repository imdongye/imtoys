//
//  for test pbr
//	2022-11-28 / im dong ye
//
//	TODO list:
//
//

#ifndef APP_PBR_H
#define APP_PBR_H

namespace lim
{
	class AppPbr: public AppBase
	{
	public:
		inline static constexpr const char *APP_NAME = "pbr assignment";
		inline static constexpr const char *APP_DISC = "ggx beckman";
	private:
		Camera* camera;
		Program* prog;

		int nr_rows    = 3;
		int nr_columns = 3;
		float spacing = 2.5;

		/* lights */
		glm::vec3 light_position = glm::vec3(-10.0f, 10.0f, 10.0f);
		glm::vec3 light_color = glm::vec3(300.0f, 300.0f, 300.0f);

		unsigned int sphere_vao = 0;
		unsigned int index_count = 0;

		bool is_dragging = false;
		double xold, yold;

	public:
		AppPbr(): AppBase(1280, 720, APP_NAME)
		{
			stbi_set_flip_vertically_on_load(true);
			camera = new Camera( glm::vec3(0, 0, 3), scr_width/(float)scr_height );
			camera->fovy = 45.f;
			camera->updateProjMat();

			prog = new Program("pbr");
			prog->setHomeDir("impbr").attatch("1.1.pbr.vs").attatch("1.1.pbr.fs").link();

			GLuint pid = prog->use();
			setUniform(pid, "albedo", glm::vec3(0.5, 0, 0));
			setUniform(pid, "ao", 1.0f);
			

			/* initialize static shader uniforms before rendering */
			setUniform(pid, "projection", camera->proj_mat);
		}
		~AppPbr()
		{
		}
	private:
		virtual void update() final
		{
			// input
			// -----
			processInput(window);

			// render
			// ------
			glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			GLuint pid = prog->use();
			setUniform(pid, "view", camera->view_mat);
			setUniform(pid, "camPos", camera->position);

			// render rows*column number of spheres with varying metallic/roughness values scaled by rows and columns respectively
			glm::mat4 model = glm::mat4(1.0f);
			for( int row = 0; row < nr_rows; ++row ) {
				setUniform(pid, "metallic", row/(float)nr_rows);

				for( int col = 0; col < nr_columns; ++col ) {
					// we clamp the roughness to 0.05 - 1.0 as perfectly smooth surfaces (roughness of 0.0) tend to look a bit off
					// on direct lighting.
					setUniform(pid, "roughness", glm::clamp((float)col / (float)nr_columns, 0.05f, 1.0f));

					model = glm::mat4(1.0f);
					model = glm::translate(model, glm::vec3(
						(col - (nr_columns / 2)) * spacing,
						(row - (nr_rows / 2)) * spacing,
						0.0f
					));
					setUniform(pid, "model", model);
					renderSphere();
				}
			}

			// render light source (simply re-render sphere at light positions)
			// this looks a bit off as we use the same shader, but it'll make their positions obvious and 
			// keeps the codeprint small.
			glm::vec3 newPos = light_position + glm::vec3(sin(glfwGetTime() * 5.0) * 5.0, 0.0, 0.0);
			newPos = light_position;
			setUniform(pid, "lightPosition", newPos);
			setUniform(pid, "lightColor", light_color);

			model = glm::mat4(1.0f);
			model = glm::translate(model, newPos);
			model = glm::scale(model, glm::vec3(0.5f));
			setUniform(pid, "model", model);
			renderSphere();
		}
		virtual void renderImGui() final
		{
			//imgui_modules::ShowExampleAppDockSpace([]() {});

			//ImGui::ShowDemoWindow();

			//ImGui::Begin("state");
			//ImGui::PushItemWidth()
			//ImGui::End();
		}
	private:

		// renders (and builds at first invocation) a sphere
		// -------------------------------------------------
		void renderSphere()
		{
			if( sphere_vao == 0 ) {
				glGenVertexArrays(1, &sphere_vao);

				unsigned int vbo, ebo;
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
				index_count = indices.size();

				std::vector<float> data;
				for( unsigned int i = 0; i < positions.size(); ++i ) {
					data.push_back(positions[i].x);
					data.push_back(positions[i].y);
					data.push_back(positions[i].z);
					if( uv.size() > 0 ) {
						data.push_back(uv[i].x);
						data.push_back(uv[i].y);
					}
					if( normals.size() > 0 ) {
						data.push_back(normals[i].x);
						data.push_back(normals[i].y);
						data.push_back(normals[i].z);
					}
				}
				glBindVertexArray(sphere_vao);
				glBindBuffer(GL_ARRAY_BUFFER, vbo);
				glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
				glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
				float stride = (3 + 2 + 3) * sizeof(float);
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
				glEnableVertexAttribArray(2);
				glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(5 * sizeof(float)));
			}

			glBindVertexArray(sphere_vao);
			glDrawElements(GL_TRIANGLE_STRIP, index_count, GL_UNSIGNED_INT, 0);
		}

		void processInput(GLFWwindow *window)
		{
			static const double cameraMoveSpeed = 1.6;
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
		virtual void framebufferSizeCallback(double width, double height) final
		{
			glViewport(0, 0, width, height);
		}
		virtual void scrollCallback(double xoff, double yoff) final
		{
			camera->shiftZoom(yoff*5.f);
			camera->updateProjMat();
		}
		virtual void keyCallback(int key, int scancode, int action, int mods) final
		{
			std::cout<<ImGui::GetFrameHeight();
		}
		virtual void mouseBtnCallback(int button, int action, int mods) final
		{
			is_dragging = action ==GLFW_PRESS;
			if( is_dragging )
				xold=-1;
		}
		virtual void cursorPosCallback(double xpos, double ypos) final
		{
			static const float rotateSpeed = 0.05f;
			static float xoff, yoff;

			if( !is_dragging ) return;

			if( xold<0 ) {
				xold = xpos;
				yold = ypos;
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
