#define STB_IMAGE_IMPLEMENTATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb/stb_image.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>

#include "core/Window.h"
#include <miniBoxLog.h>
#include "helper.h"

int Window::s_glfwRefCount = 0;


Window::Window(const WindowConfig& config) : m_config(config)
{
	if (s_glfwRefCount == 0) {
		if (!glfwInit()) {
			BOX_LOG_DEBUG("BoxWindow: Failed to initialize GLFW");
			return;
		}
	}
	++s_glfwRefCount;

	// Default to OpenGL 4.6 core; adjust if you need different version/profile
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, m_config.resizable ? GLFW_TRUE : GLFW_FALSE);

	window = glfwCreateWindow(m_config.width, m_config.height, m_config.title, nullptr, nullptr);
	if (!window) {
		BOX_LOG_DEBUG("Window: Failed to create GLFW window");
		--s_glfwRefCount;
		if (s_glfwRefCount == 0) glfwTerminate();
		return;
	}

	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		BOX_LOG_WARNING("Failed to initialize GLAD");
		return;
	}



	SetVSync(m_config.vsync);
	SetIcon(window);
}

void Window::SetIcon(GLFWwindow* window)
{
	Helpers helpers;
	std::string iconPath = helpers.GetAssetPath(ICON_PATH);
	GLFWimage images[1];
	images[0].pixels = stbi_load(iconPath.c_str(), &images[0].width, &images[0].height, 0, 4); // rgba = png
	glfwSetWindowIcon(window, 1, images);
	stbi_image_free(images[0].pixels);
}




void Window::Update()
{
	if (!window)
		return;

	glfwSwapBuffers(window);


}

bool Window::ShouldClose() const
{
	if (!window)
		return true;

	return glfwWindowShouldClose(window) == GLFW_TRUE;
}

void Window::SetVSync(bool enabled)
{
	if (!window) return;
	glfwMakeContextCurrent(window);
	glfwSwapInterval(enabled ? 1 : 0);
	m_config.vsync = enabled;
}


void Window::Shutdown()
{
	if (window)
	{
		glfwDestroyWindow(window);
		window = nullptr;
	}

	if (s_glfwRefCount > 0)
	{
		--s_glfwRefCount;

		if (s_glfwRefCount == 0)
			glfwTerminate();
	}
}

bool Window::ImguiInit(GLFWwindow* window)
{
	Helpers helpers; // from Helpers.h, used to get asset paths - fonts, textures etc.
	// ImGui set up
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	
	ImGui::StyleColorsDark();

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	const char* glsl_version = "#version 460 core";
	ImGui_ImplOpenGL3_Init(glsl_version);

	// Fonts
	ImFontConfig fontconfig;
	fontconfig.MergeMode = true;
	fontconfig.PixelSnapH = true;
	static const ImWchar ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };


	fontconfig.GlyphOffset = ImVec2(0.0f, 1.0f);
	std::string fontPath = helpers.GetAssetPath(FONT_PATH_MAIN_REL); // comic sans font
	io.Fonts->AddFontFromFileTTF(fontPath.c_str(), FONT_SIZE);

	std::string AFfontPath = helpers.GetAssetPath(FA_SOLID_PATH);
	io.Fonts->AddFontFromFileTTF(AFfontPath.c_str(), FONT_SIZE, &fontconfig, ranges);

	m_imguiInitialized = true;
	BOX_LOG_INFO("ImGui initialized successfully");

	return true;
}

void Window::BeginFrame()
{
	// New Frame
	if (!m_imguiInitialized)
		return;

	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void Window::RenderImGui()
{
	if (!m_imguiInitialized)
		return;

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void Window::ImGuiShutdown()
{
	if (!m_imguiInitialized)
		return;

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	m_imguiInitialized = false;
}
