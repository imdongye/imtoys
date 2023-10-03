//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
//
//	TODO list:
//	1. crtp로 참조 줄이기,
//	2. 헤더파일include 중복관리
//	3. dnd 여러개들어왔을때 모델파일만 골라서 입력받게 조건처리
//	4. figure out [this](auto&&... args) -> decltype(auto) { return this->fn(std::forward<decltype(args)>(args)...); }
//	5. https://github.com/AndrewBelt/osdialog 로 multi platform open file dialog
//	6. SHADER파일 정리
//

#ifndef __app_simplification_h_
#define __app_simplification_h_

#include <limbrary/application.h>
#include <limbrary/program.h>
#include <limbrary/model_view/light.h>
#include <limbrary/model_view/model.h>
#include <limbrary/model_view/camera_auto.h>
#include <limbrary/model_view/scene.h>


namespace lim
{
	class AppSimplification : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME =  "Cloth-Simplificator";
		inline static constexpr CStr APP_DIR  =  "im_simplification";
		inline static constexpr CStr APP_DISC =  "simplify and bake normal map";

	private:
		bool is_same_camera = true;

		int selected_prog_idx = 0;
		std::vector<Program> programs;
		std::vector<const char*> shader_names;

		bool simplify_trigger = false;
		bool bake_trigger = false;
		int src_vp_idx = 0;
		int dst_vp_idx = 1;
		int last_focused_vp_idx = 0;
		
		Light light;
		Model ground;

		int nr_viewports = 0;
		std::vector<ViewportWithCamera> viewports;
		// scene에서 주소값을 사용하기때문에 
		std::vector<Model*> models;
		std::vector<Scene> scenes;

		const char* export_path = "exports/simp_rst";

	public:
		AppSimplification();
		~AppSimplification();
	private:
		void addEmptyViewport();
		void subViewport(int vpIdx);
		void doImportModel(std::string_view path, int vpIdx);
		void doExportModel(size_t pIndex, int vpIdx);
		void doSimplifyModel(float lived_pct = 0.8f, int version = 0, int agressiveness = 7, bool verbose = true);
		void doBakeNormalMap(int texSize);

	private:
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void mouseBtnCallback(int button, int action, int mods) override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
