#include "app_gen_mesh.h"
#include <stb_image.h>
#include <limbrary/model_view/mesh_maked.h>
#include <limbrary/model_view/renderer.h>
#include <glm/gtx/transform.hpp>
#include <imgui.h>
#include <limbrary/asset_lib.h>

namespace lim
{
	void AppGenMesh::addMeshToScene(Mesh* ms) {
		Model* md = new Model("sphere");
		md->addOwn(ms);
		md->root.addMsMat(ms, &default_mat);
		scene.addOwn(md);
	}
}

namespace lim
{
	AppGenMesh::AppGenMesh() : AppBase(1200, 780, APP_NAME), viewport("viewport##gen_mesh", new FramebufferMs())
	{
		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_FRONT);
		// glPolygonMode(GL_FRONT, GL_LINE);
		viewport.camera.spd_free_move = 4.f;


		program.name = "debugging";
		program.home_dir = APP_DIR;
		program.attatch("assets/shaders/mvp.vs").attatch("debug.fs").link();
		default_mat.prog = &program;
		default_mat.set_prog = [this](const Program& prog) {
			prog.setUniform("time", vs_t);
			prog.setTexture("uvgridTex", debugging_tex.tex_id);
		};


		/* gen models */
		addMeshToScene(new MeshSphere(1.f, 10, 25));
		addMeshToScene(new MeshEnvSphere(10));
		addMeshToScene(new MeshDonut(50, 25));
		addMeshToScene(new MeshCapsule(50, 25));
		addMeshToScene(new MeshIcoSphere(0));
		addMeshToScene(new MeshIcoSphere(1));
		addMeshToScene(new MeshIcoSphere(2));
		addMeshToScene(new MeshIcoSphere(3));
		addMeshToScene(new MeshCubeSphere(2));
		addMeshToScene(new MeshCubeSphere2(5));
		addMeshToScene(new MeshQuad());
		addMeshToScene(new MeshCube());
		addMeshToScene(new MeshCylinder(20));

		Model* md = new Model();
		md->importFromFile("assets/models/objs/spot.obj");
		md->setUnitScaleAndPivot();
		md->setSameMat(&default_mat);

		scene.addOwn(md);

		md = new Model();
		md->importFromFile("assets/models/objs/Wooden Crate.obj");
		md->setUnitScaleAndPivot();
		md->setSameMat(&default_mat);
		scene.addOwn(md);


		const float interModels = 3.5f;
		const float biasModels = -interModels * scene.models.size() / 2.f;

		for( int i = 0; i < scene.models.size(); i++ )
		{
			scene.own_mds[i]->tf->pos = {biasModels + interModels * i, 0, 0};
			scene.own_mds[i]->tf->update();
		}

		addMeshToScene(new MeshPlane());
		scene.own_mds.back()->tf->pos = {0, -3.5, 0};
		scene.own_mds.back()->tf->scale = glm::vec3{50.f};
		scene.own_mds.back()->tf->update();

		scene.addRef(&light);


		debugging_tex.s_wrap_param = GL_REPEAT;
		debugging_tex.initFromFile("assets/images/uv_grid.jpg", true);

	}
	AppGenMesh::~AppGenMesh()
	{
	}

	void AppGenMesh::update()
	{
		/* render to fbo in viewport */
		render(viewport.getFb(), viewport.camera, scene);

		// clear backbuffer
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		glDisable(GL_MULTISAMPLE);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, fb_width, fb_height);
		glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	void AppGenMesh::updateImGui()
	{
		ImGui::DockSpaceOverViewport();


		viewport.drawImGui();
		

		/* controller */
		ImGui::Begin("controller##genMesh");
		ImGui::SliderFloat("vs_t", &vs_t, 0.f, 1.f);
		ImGui::Text("<light>");
		const static float litThetaSpd = 70 * 0.001;
		const static float litPiSpd = 360 * 0.001;
		bool isLightDraged = false;
		isLightDraged |= ImGui::DragFloat("light phi", &light.tf.phi, litPiSpd, 0, 360, "%.3f");
		isLightDraged |= ImGui::DragFloat("light theta", &light.tf.theta, litThetaSpd, 0, 180, "%.3f");
		if( isLightDraged ) {
			light.tf.updateWithRotAndDist();
		}
		ImGui::Text("pos %f %f %f", light.tf.pos.x, light.tf.pos.y, light.tf.pos.z);
		ImGui::End();

		/* state view */
		ImGui::Begin("camera##genMesh");
		static float rad = F_PI;
		ImGui::SliderAngle("rad", &rad);
		ImGui::End();
	}
}