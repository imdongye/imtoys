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
#include "viewport_pack.h"


namespace lim
{
	class AppSimplification : public AppBase
	{
	public:
		inline static constexpr CStr APP_NAME =  "Cloth-Simplificator";
		inline static constexpr CStr APP_DIR  =  "imsimplification";
		inline static constexpr CStr APP_DISC =  "simplify and bake normal map";

	private:
		bool isSameCamera = false;
		float cameraMoveSpeed = 1.6;
		Light light;
		Model ground;

		int selectedProgIdx = 0;
		std::vector<Program *> programs;

		int lastFocusedVpIdx = 0;
		bool simplifyTrigger = false;
		bool bakeTrigger = false;
		int fromVpIdx = 0;
		int toVpIdx = 1;
		ViewportPackage vpPackage;

		float simp_time=0;

		const char *exportPath = "imsimplification/result/";

	public:
		AppSimplification();
		~AppSimplification();
	private:
		void addEmptyViewport();
		void loadModel(std::string_view path, int vpIdx);
		void exportModel(size_t pIndex, int vpIdx);
		void simplifyModel(float lived_pct = 0.8f, int version = 0, int agressiveness = 7, bool verbose = true);
		void bakeNormalMap();

	private:
		virtual void update() override;
		virtual void renderImGui() override;
		virtual void keyCallback(int key, int scancode, int action, int mods) override;
		virtual void mouseBtnCallback(int button, int action, int mods) override;
		virtual void dndCallback(int count, const char **paths) override;
	};
}

#endif
