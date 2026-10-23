#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>


#include "core/Window.h"
#include <miniBoxLog.h>
#include <glad/glad.h>
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
