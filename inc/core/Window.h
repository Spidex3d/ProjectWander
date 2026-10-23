#pragma once
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>	

struct WindowConfig {

	int width = 1280;
	int height = 720;
	const char* title = "Project Wander";
	bool resizable = true;
	bool vsync = true;
};

class Window
{
public:

	explicit Window(const WindowConfig& config);

	GLFWwindow* GetWindow() const { return window; }

	int GetWindowWidth() const { return m_config.width; }
	int GetWindowHeight() const { return m_config.height; }

	void SetIcon(GLFWwindow* window); // set window icon from image file

	bool ShouldClose() const;

	void Update();
	void Shutdown();

	bool ImguiInit(GLFWwindow* window);
	void BeginFrame();
	void RenderImGui(); // finish ImGui frame and render
	void ImGuiShutdown();




	void SetVSync(bool enabled);


private:
	GLFWwindow* window = nullptr;

	WindowConfig m_config;

	bool m_imguiInitialized = false;

	// Simple refcount so multiple Window instances don't re-init/terminate GLFW
	static int s_glfwRefCount; // static member declaration


};
