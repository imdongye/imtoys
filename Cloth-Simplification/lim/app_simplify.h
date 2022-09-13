//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. crtp로 참조 줄이기, 
//	2. 헤더파일include 중복관리
//	4. figure out [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
//	3. dnd 여러개들어왔을때 모델파일만 골라서 입력받게 조건처리
//

#ifndef SIMPLYFY_APP_H
#define SIMPLYFY_APP_H

namespace lim
{
	class SimplifyApp: public AppBase
	{
	private:
		float cameraMoveSpeed;
		Light light;
		Camera* cameras[2];
		Model* models[2];
		Scene* scenes[2];
		Viewport* viewports[2];
		std::vector<Program*> programs;

	public:
		SimplifyApp(): AppBase(1200, 960), cameraMoveSpeed(1.6f), light()
		{
			programs.push_back(new Program("Normal Dot View"));
			programs.back()->attatch("shader/posnoruv.vs").attatch("shader/ndv.fs").link();

			programs.push_back(new Program("Normal Dot Light"));
			programs.back()->attatch("shader/posnoruv.vs").attatch("shader/ndl.fs").link();

			programs.push_back(new Program("Diffuse"));
			programs.back()->attatch("shader/posnoruv.vs").attatch("shader/diffuse.fs").link();

			programs.push_back(new Program("Uv"));
			programs.back()->attatch("shader/posnoruv.vs").attatch("shader/uv.fs").link();

			programs.push_back(new Program("Bump"));
			programs.back()->attatch("shader/posnoruv.vs").attatch("shader/bump.fs").link();

			programs.push_back(new Program("Shadowed"));
			programs.back()->attatch("shader/shadowed.vs").attatch("shader/shadowed.fs").link();

			programs.push_back(new Program("Uv View"));
			programs.back()->attatch("shader/uv_view.vs").attatch("shader/uv_view.fs").link();

			models[0] = new Model("archive/dwarf/Dwarf_2_Low.obj", programs[0], true);
			models[1] = nullptr;

			scenes[0] = new Scene(programs[0], light);
			scenes[0]->setModel(models[0]);

			scenes[1] = new Scene(programs[0], light);
			scenes[1]->setModel(models[0]);

			cameras[0] = new Camera(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0, 0, -1), scr_width/(float)scr_height);
			cameras[1] = new Camera(glm::vec3(0.0f, 1.0f, 3.0f), glm::vec3(0, 0, -1), scr_width/(float)scr_height);

			Viewport* originalView = new Viewport(cameras[0]);
			viewports[0] = originalView;

			Viewport* simplifiedView = new Viewport(cameras[1]);
			viewports[1] = simplifiedView;

			init_callback();
			imgui_modules::initImGui(window);
		}
		~SimplifyApp()
		{
			for( Camera* camera : cameras ) delete camera;
			for( Model* model : models ) delete model;
			for( Scene* scene : scenes ) delete scene;
			for( Viewport* viewport : viewports ) delete viewport;
			for( Program* program : programs ) delete program;

			imgui_modules::destroyImGui();
		}
	private:
		virtual void update() final
		{
			processInput();

			//scenes[0]->render(0, scr_width, scr_height, cameras[0]);
			scenes[0]->render(viewports[0]);
			scenes[1]->render(viewports[1]);

			renderImGui();

			/* render to screen */
			//scenes.back()->render(0, scr_width, scr_height, mainCamera);
		}
		void renderImGui()
		{
			imgui_modules::beginImGui();
			imgui_modules::ShowExampleAppDockSpace();

			ImGui::ShowDemoWindow();

			viewports[0]->renderImGui();
			viewports[1]->renderImGui();

			ImGui::Begin("Simplify Options");
			{
				static float pct = 0.8f;
				ImGui::SliderFloat("percent", &pct, 0.0f, 1.0f);

				if( ImGui::Button("Simplify") )
				{
					if( models[1] != nullptr ) delete models[1];
					models[1] = fqms::simplifyModel(scenes[0]->model, pct);

					scenes[1]->setModel(models[1]);
				}
				ImGui::SameLine();
				ImGui::Text("target triangles = %d", static_cast<int>(scenes[0]->model->trianglesNum*pct));

				ImGui::Text("%.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			} ImGui::End();


			ImGui::Begin("Viewing Options");
			{
				static bool isSameCamera = false;
				if( ImGui::Checkbox("use same camera", &isSameCamera) )
				{
					if( isSameCamera )
						viewports[1]->camera = cameras[0];
					else
						viewports[1]->camera = cameras[1];
				}

				ImGui::SliderFloat("move speed", &cameraMoveSpeed, 1.0f, 3.0f);

				static int prog_idx = 0;
				std::vector<const char*> comboList;
				for( Program* prog : programs )
					comboList.push_back(prog->name.c_str());
				if( ImGui::Combo("shader", &prog_idx, comboList.data(), comboList.size()) )
				{
					for( Scene* scene : scenes )
					{
						scene->model->program = programs[prog_idx];
						scene->ground->program = programs[prog_idx];
					}
				}
				const float yawSpd = 360*0.001;
				if( ImGui::DragFloat("light yaw", &light.yaw, yawSpd, -10, 370, "%.3f") )
					light.updateMembers();
				const float pitchSpd = 70*0.001;
				if( ImGui::DragFloat("light pitch", &light.pitch, pitchSpd, -100, 100, "%.3f") )
					light.updateMembers();

				ImGui::Text("pos %f %f %F", light.position.x, light.position.y, light.position.z);
			} ImGui::End();

			ImGui::SetNextWindowSize(ImVec2{0,0});
			ImGui::Begin("shadowMap");
			{
				ImGui::Image(reinterpret_cast<void*>(light.shadowMap.getRenderedTex()), ImVec2{256, 256}, ImVec2{0, 1}, ImVec2{1, 0});
			}ImGui::End();

			ImGui::Begin("Model Status");
			{
				ImGui::Text("<Original model>");
				lim::Model& ori = *(scenes[0]->model);
				ImGui::Text("name : %s", ori.name.c_str());
				ImGui::Text("#verteces : %u", ori.verticesNum);
				ImGui::Text("#triangles : %u", ori.trianglesNum);
				ImGui::Text("#meshes : %lu", ori.meshes.size());
				ImGui::Text("#textures : %lu", ori.textures_loaded.size());
				lim::Model& simp = *(scenes[1]->model);
				ImGui::NewLine();
				ImGui::Text("<Simplified model>");
				ImGui::Text("#verteces : %u", simp.verticesNum);
				ImGui::Text("#triangles : %u", simp.trianglesNum);
			} ImGui::End();

			imgui_modules::endImGui(scr_width, scr_height);
		}
		void init_callback()
		{
			//wData.drop_callback = std::bind(&SimplifyApp::drop_callback, this, std::placeholders::_1, std::placeholders::_2);
			wData.win_size_callback = [this](int width, int height) {
				return this->win_size_callback(width, height); };

			wData.key_callback = [this](int key, int scancode, int action, int mods) {
				return this->key_callback(key, scancode, action, mods); };

			wData.mouse_btn_callback = [this](int button, int action, int mods) {
				return this->mouse_btn_callback(button, action, mods); };

			wData.scroll_callback = [this](double xOffset, double yOffset) {
				return this->scroll_callback(xOffset, yOffset); };

			wData.cursor_pos_callback = [this](double xPos, double yPos) {
				return this->cursor_pos_callback(xPos, yPos); };

			wData.drop_callback = [this](int count, const char** path) {
				return this->drop_callback(count, path); };
		}
		void processInput()
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;

				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS )
				{
					float multiple = 1.0f;
					if( glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS )
						multiple = 1.3f;
					if( glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::FORWARD, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::BACKWARD, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::LEFT, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::RIGHT, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::UP, deltaTime, cameraMoveSpeed*multiple);
					if( glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS )
						camera.move(Camera::MOVEMENT::DOWN, deltaTime, cameraMoveSpeed*multiple);
				}
			}
		}
		void key_callback(int key, int scancode, int action, int mode)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS
				   && key == GLFW_KEY_Z )
				{
					if( action == GLFW_PRESS )
					{
						camera.readyPivot();
						camera.updatePivotViewMat();
					}
					else if( action == GLFW_RELEASE )
					{
						camera.readyFree();
						camera.updateFreeViewMat();
					}
				}
			}
			if( key == GLFW_KEY_ESCAPE && action == GLFW_PRESS )
				glfwSetWindowShouldClose(window, GL_TRUE);
		}
		void cursor_pos_callback(double xPos, double yPos)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				const float w = scr_width;
				const float h = scr_height;

				float xoff = xPos - vp->cursor_pos_x;
				float yoff = vp->cursor_pos_y - yPos; // inverse

				if( glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) )
				{
					if( glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS )
					{
						if( glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS )
						{
							camera.shiftDist(xoff/w*160.f);
							camera.shiftZoom(yoff/h*160.f);
						}
						else
						{
							camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
						}
						camera.updatePivotViewMat();
					}
					else
					{
						camera.rotateCamera(xoff/w*160.f, yoff/h*160.f);
						camera.updateFreeViewMat();
					}

				}
				vp->cursor_pos_x = xPos; vp->cursor_pos_y = yPos;
			}
		}
		void mouse_btn_callback(int button, int action, int mods)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->hovered == false ) continue;
				Camera& camera = *vp->camera;
				if( action == GLFW_PRESS )
				{
					double xpos, ypos;
					glfwGetCursorPos(window, &xpos, &ypos);
					vp->cursor_pos_x = xpos;
					vp->cursor_pos_y = ypos;
				}
			}
		}
		void scroll_callback(double xoff, double yoff)
		{
			for( Viewport* vp : viewports )
			{
				if( vp->focused == false ) continue;
				Camera& camera = *vp->camera;
				camera.shiftZoom(yoff*10.f);
				camera.updateProjMat();
			}
		}
		void win_size_callback(int width, int height)
		{
			scr_width = width;
			scr_height = height;
		}
		void drop_callback(int count, const char** paths)
		{
			for( int i = 0; i < count; i++ )
			{
				Program* usedProg = models[0]->program;
				delete models[0];
				models[0] = new Model(paths[i], usedProg, true);
				scenes[0]->setModel(models[0]);
				scenes[1]->setModel(models[0]);
				break;
			}
		}
	};

}

#endif
