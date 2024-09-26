/*
    2024-09-21 / imdongye

    small size implimantations

*/

#include <limbrary/model_view/viewport_with_cam.h>
#include <limbrary/application.h>
#include <limbrary/tools/text.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <limbrary/tools/limgui.h>
#include <imguizmo/ImGuizmo.h>

#include <limbrary/using_in_cpp/glm.h>
using namespace lim;


Viewport::Viewport(IFramebuffer* createdFB, const char* _name)
{
	assert(createdFB);
	name = fmtStrToBuf("%s##%s", _name, AppBase::g_app_name);
	own_framebuffer = createdFB;
	own_framebuffer->resize({256, 256}); // default size
}


ViewportWithCam::ViewportWithCam(IFramebuffer* createdFB, const char* _name)
	: Viewport(createdFB, _name)
{
}

void ViewportWithCam::movePosFormMouse(vec3& target) const
{
	const vec3 mouseRay = getMousePosRayDir();
	const vec3 toTarget = target - camera.pos;
	float depth = glm::dot(camera.front, toTarget)/glm::dot(camera.front, mouseRay);
	if( is_hovered ) {
		depth += ImGui::GetIO().MouseWheel * 0.1f;
	}
	target = depth*mouseRay+camera.pos;
}




//
//	imgui part
//
namespace
{
	std::function<void(ViewportWithCam&)> vp_guizmo_hook = nullptr;
}

void Viewport::drawImGui()
{
	IFramebuffer& fb = *own_framebuffer;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0, 0});
	ImGui::SetNextWindowSize(
		{(float)fb.size.x, (float)fb.size.y+ImGui::GetFrameHeight()},
		(window_mode==Viewport::WM_FIXED_SIZE) ? ImGuiCond_Always : ImGuiCond_Once
	);

	ImGuiWindowFlags vpWinFlag = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar;


	if( window_mode==Viewport::WM_FIXED_RATIO ) {
		static bool is_resize_with_corner = false;
		
		vpWinFlag |= ImGuiWindowFlags_NoDocking;
		ImGui::SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, FLT_MAX), [](ImGuiSizeCallbackData *data) {
			if( isSame(data->CurrentSize, data->DesiredSize) ) {
				return;
			}
			const int cursorId = ImGui::GetMouseCursor();
			const float frameHeight = ImGui::GetFrameHeight();
			Viewport& ivp = *(Viewport*)(data->UserData);

			vec2 dstContentSize = toGlm(data->DesiredSize);
			dstContentSize.y -= frameHeight;
			
			if( cursorId == ImGuiMouseCursor_ResizeNWSE ) {
				const float newAspect = dstContentSize.x / dstContentSize.y;
				if( ivp.fixed_aspect > newAspect ) {
					dstContentSize.x = ivp.fixed_aspect * dstContentSize.y;
				}
				else {
					dstContentSize.y = dstContentSize.x / ivp.fixed_aspect;
				}
			}
			else if( cursorId == ImGuiMouseCursor_ResizeNS ) {
				dstContentSize.x = ivp.fixed_aspect * dstContentSize.y;
			}
			else { // ImGuiMouseCursor_ResizeEW
				dstContentSize.y = dstContentSize.x / ivp.fixed_aspect;
			}

			dstContentSize.y += frameHeight;
			data->DesiredSize = toIg(dstContentSize);
		}, (void*)this);
	}
	else if( window_mode==Viewport::WM_FIXED_SIZE ) {
		vpWinFlag |= ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoResize;
	}



	ImGui::Begin(name.c_str(), &is_opened, vpWinFlag);
	
	// for ignore draging
	const bool isWindowActivated = ImGui::IsItemActive(); // when resize window
	const bool isHoveredOnTitle = ImGui::IsItemHovered(); // when move window


	content_size = toGlm(ImGui::GetContentRegionAvail());
	fb_size = content_size;
	content_pos = toGlm(ImGui::GetCursorScreenPos());

	// ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelX);
	// ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
	ImGui::Image(texIdToIg(fb.getRenderedTexId()), toIg(fb.size), {0, 1}, {1, 0});

	is_hidden = LimGui::IsWindowHidden();
	is_focused = ImGui::IsWindowFocused();
	is_hovered = ImGui::IsItemHovered();
	is_dragged = isWindowActivated && !isHoveredOnTitle && is_focused && ImGui::IsMouseDown(0);


	mouse_pos = toGlm(ImGui::GetMousePos()) - content_pos;
	mouse_uv_pos = mouse_pos/content_size;
	
	if( vp_guizmo_hook ) {
		vp_guizmo_hook(*(ViewportWithCam*)this);
		is_dragged &= !ImGuizmo::IsUsingAny();
	}
	if( is_dragged ) {
		ImGui::SetMouseCursor(7);
	}

	ImGui::End();
	ImGui::PopStyleVar();

	// update size
	if( fb_size != fb.size ) {
		// This is sometimes negative
		assert( fb_size.x>0 && fb_size.y>0 );
		fb.resize(fb_size);
		is_size_changed = true;
	}
	else {
		is_size_changed = false;
	}
}


void ViewportWithCam::drawImGuiAndUpdateCam(std::function<void(ViewportWithCam&)> guizmoHook)
{
	vp_guizmo_hook = guizmoHook;
	Viewport::drawImGui();
	vp_guizmo_hook = nullptr;

	camera.updateFrom(*this);
}