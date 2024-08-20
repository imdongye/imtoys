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
		ms->initGL(false);
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
		// glEnable(GL_CULL_FACE);
		// glCullFace(GL_BACK);
		// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
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
		addMeshToScene(new MeshQuad());
		addMeshToScene(new MeshCube());
		addMeshToScene(new MeshSphere(1.f, 10, 4, 5));
		addMeshToScene(new MeshEnvSphere(1.f, 10));
		addMeshToScene(new MeshIcoSphere(1.f, 0));
		addMeshToScene(new MeshIcoSphere(1.f, 1));
		addMeshToScene(new MeshIcoSphere(1.f, 2));
		addMeshToScene(new MeshIcoSphere(1.f, 3));
		addMeshToScene(new MeshCubeSphere(1.f, 3));
		addMeshToScene(new MeshCubeSphereSmooth(1.f, 3));
		addMeshToScene(new MeshCylinder());
		addMeshToScene(new MeshCapsule(1.f, 2.f, 8, 5, true, false));
		addMeshToScene(new MeshDonut(0.5f, 0.2f, 50, 25));

		Model* md = new Model();
		md->importFromFile("assets/models/objs/spot.obj", false, true, 1.f, glm::vec3(0));
		md->setSameMat(&default_mat);

		scene.addOwn(md);

		md = new Model();
		md->importFromFile("assets/models/objs/Wooden Crate.obj", false, true, 1.f, glm::vec3(0));
		md->setSameMat(&default_mat);
		scene.addOwn(md);


		const float interModels = 1.2f;
		const float biasModels = -interModels * scene.own_mds.size() / 2.f;

		for( int i = 0; i < scene.own_mds.size(); i++ )
		{
			scene.own_mds[i]->root.tf.pos = {biasModels + interModels * i, 0, 0};
			scene.own_mds[i]->root.tf.update();
			scene.own_mds[i]->root.updateGlobalTransform();
		}

		addMeshToScene(new MeshPlane());
		scene.own_mds.back()->root.tf.pos = {0, -1.f, 0};
		scene.own_mds.back()->root.tf.scale = glm::vec3{20.f};
		scene.own_mds.back()->root.tf.update();
		scene.own_mds.back()->root.updateGlobalTransform();

		scene.addOwn(new LightDirectional());


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
		const static float litThetaSpd = 70 * 0.001f;
		const static float litPiSpd = 360 * 0.001f;
		bool isLightDraged = false;
		LightDirectional& light = *(LightDirectional*)scene.own_lits[0];
		isLightDraged |= ImGui::DragFloat("light phi", &light.tf.phi, litPiSpd, 0, 360, "%.3f");
		isLightDraged |= ImGui::DragFloat("light theta", &light.tf.theta, litThetaSpd, 0, 180, "%.3f");
		if( isLightDraged ) {
			light.tf.updateWithRotAndDist();
		}
		ImGui::Text("pos %f %f %f", light.tf.pos.x, light.tf.pos.y, light.tf.pos.z);
		ImGui::End();

		/* state view */
		ImGui::Begin("camera##genMesh");
		static float rad = glm::pi<float>();
		ImGui::SliderAngle("rad", &rad);
		ImGui::End();
	}
}