//
//  for test simplification and normal map baking.
//	2022-08-26 / im dong ye
//
//	Todo:
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
#include <limbrary/3d/light.h>
#include <limbrary/3d/model.h>
#include <limbrary/3d/viewport_with_cam.h>
#include <limbrary/3d/scene.h>


namespace lim
{
	class AppSimplification : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME =  "Cloth-Simplificator";
		inline static constexpr CStr APP_DIR  =  "im_simplification/";
		inline static constexpr CStr APP_INFO =  "simplify and bake normal map";

	private:
		bool is_same_camera = true;

		int selected_prog_idx = 0;
		std::vector<Program*> programs;
		std::vector<const char*> shader_names;

		bool simplify_trigger = false;
		bool bake_trigger = false;
		int src_vp_idx = 0;
		int dst_vp_idx = 1;
		int last_focused_vp_idx = 0;
		
		ModelData ground;

		int nr_viewports = 0;
		std::vector<ViewportWithCam*> viewports;
		std::vector<Scene*> scenes;

		const char* export_path = "exports/simp_rst";

	public:
		AppSimplification();
		~AppSimplification();
	private:
		void addEmptyViewport();
		void subViewport(int vpIdx);
		void doImportModel(const char* path, int vpIdx);
		void doExportModel(size_t pIndex, int vpIdx);
		void doSimplifyModel(float lived_pct = 0.8f, int version = 0, int agressiveness = 7, bool verbose = true);
		void doBakeNormalMap(int texSize);
		void setProg(int idx);
	private:
		virtual void update() override;
		virtual void updateImGui() override;
	};
}

#endif
