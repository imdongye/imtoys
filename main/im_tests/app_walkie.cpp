#include "app_walkie.h"
#include <limbrary/tools/log.h>
#include <imgui.h>

using namespace lim;

AppWalkie::AppWalkie() : AppBase(1200, 780, APP_NAME)
{
	key_callbacks[this] = [](int key, int scancode, int action, int mods) {
		// log::pure("%d\n", key);
		// log::pure("%d\n", key);
		// log::warn("%d\n", key);
		// log::err("%d\n", key);
	};
}
AppWalkie::~AppWalkie()
{
}
void AppWalkie::update() 
{
	glEnable(GL_DEPTH_TEST);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, fb_width, fb_height);
	glClearColor(0.05f, 0.09f, 0.11f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}
void AppWalkie::updateImGui()
{
	ImGui::DockSpaceOverViewport();

	ImGui::Begin("talk##template");

	// Note: we are using a fixed-sized buffer for simplicity here. See ImGuiInputTextFlags_CallbackResize
	// and the code in misc/cpp/imgui_stdlib.h for how to setup InputText() for dynamically resizing strings.
	static char text[1024 * 16] =
		"/*\n"
		" The Pentium F00F bug, shorthand for F0 0F C7 C8,\n"
		" the hexadecimal encoding of one offending instruction,\n"
		" more formally, the invalid operand with locked CMPXCHG8B\n"
		" instruction bug, is a design flaw in the majority of\n"
		" Intel Pentium, Pentium MMX, and Pentium OverDrive\n"
		" processors (all in the P5 microarchitecture).\n"
		"*/\n\n"
		"label:\n"
		"\tlock cmpxchg8b eax\n";

	static ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput | ImGuiInputTextFlags_CtrlEnterForNewLine;
	// ImGui::CheckboxFlags("ImGuiInputTextFlags_ReadOnly", &flags, ImGuiInputTextFlags_ReadOnly);
	// ImGui::CheckboxFlags("ImGuiInputTextFlags_AllowTabInput", &flags, ImGuiInputTextFlags_AllowTabInput);
	// ImGui::CheckboxFlags("ImGuiInputTextFlags_CtrlEnterForNewLine", &flags, ImGuiInputTextFlags_CtrlEnterForNewLine);
	ImGui::InputTextMultiline("##source", text, IM_ARRAYSIZE(text), ImVec2(-FLT_MIN, ImGui::GetTextLineHeight() * 16), flags);

	ImGui::Text(text);

	ImGui::End();
}