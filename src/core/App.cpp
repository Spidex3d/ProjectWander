#include "core/App.h"
#include <miniBoxLog.h>
#include <GLFW/glfw3.h>	
#include <core/Window.h>


App::App() = default;
App::~App() = default;



bool App::Init()
{
	WindowConfig config;
	config.width = 1280;
	config.height = 720;
	config.title = "TextureBox";
	config.resizable = true;
	config.vsync = true;

	m_window = std::make_unique<Window>(config);

	if (!m_window->GetWindow())
	{
		BOX_LOG_DEBUG("App: Failed to create Window");
		m_window.reset();
		return false;
	}

	m_isRunning = true;

	return true;
}

int App::Run()
{
	GLFWwindow* nativeWindow = m_window->GetWindow();

	while (!glfwWindowShouldClose(nativeWindow))
	{

		glClearColor(0.15f, 0.18f, 0.22f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glfwSwapBuffers(nativeWindow);
		glfwPollEvents();
	}

	return 0;
}
