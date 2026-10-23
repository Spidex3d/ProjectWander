#include "core/App.h"
#include <miniBoxLog.h>
#include <GLFW/glfw3.h>	
#include <core/Window.h>
#include <imgui/imgui.h>

#include <world/Terrain.h>


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


	if (!m_window->ImguiInit(m_window->GetWindow()))
	{
		BOX_LOG_DEBUG("App: Failed to initialize ImGui");
		return false;
	}

	// Capture mouse for camera movement
	glfwSetInputMode(m_window->GetWindow(),	GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	m_firstMouse = true;
	m_lastFrame = static_cast<float>(glfwGetTime());


	// Set up the exploration camera
	m_camera.Mode = Camera::CameraMode::Play;
	m_camera.Projection = Camera::ProjectionMode::Perspective;
	//m_camera.SetPositionYawPitch(glm::vec3(0.0f, 15.0f, 25.0f), -90.0f, -25.0f);

	m_camera.SetPositionYawPitch(
		glm::vec3(0.0f, 180.0f, 200.0f),
		-90.0f,
		-45.0f
	);

	// Create terrain shader
	m_terrainShader = std::make_unique<Shader>(
		"assets/shaders/terrain.vert",
		"assets/shaders/terrain.frag"
	);

	// Load terrain rock material
	if (!m_rockMaterial.Load(
		"assets/materials/Blue_Rock.mat"))
	{
		BOX_LOG_ERROR("Failed to load Blue_Rock material");
		return false;
	}

	if (!m_terrainManager.Initialize())
	{
		BOX_LOG_DEBUG("Failed to initialize terrain chunks");
		return false;
	}

	m_isRunning = true;

	return true;
}

int App::Run()
{
	GLFWwindow* nativeWindow = m_window->GetWindow();

	const glm::mat4 view = m_camera.GetViewMatrix();

	const float aspect = static_cast<float>(m_window->GetWindowWidth()) / m_window->GetWindowHeight();

	const glm::mat4 projection = m_camera.GetProjectionMatrix(aspect);

	


	while (!glfwWindowShouldClose(nativeWindow))
	{
		glfwPollEvents();

		// Calculate delta time
		float currentFrame = static_cast<float>(glfwGetTime());

		m_deltaTime = currentFrame - m_lastFrame;
		m_lastFrame = currentFrame;

		// Process camera input
		ProcessInput(nativeWindow);
		ProcessMouse(nativeWindow);

		// Update terrain chunks around the camera
		m_terrainManager.Update(m_camera.Position);

		// Get current framebuffer dimensions
		int width = 0;
		int height = 0;

		glfwGetFramebufferSize(nativeWindow, &width, &height);

		// Handle minimised windows
		if (width == 0 || height == 0)
		{
			continue;
		}

		glViewport(0, 0, width, height);

		// Clear the screen
		glEnable(GL_DEPTH_TEST);

		glClearColor(0.53f, 0.75f, 0.92f, 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Camera matrices
		const glm::mat4 view = m_camera.GetViewMatrix();

		const float aspect =
			static_cast<float>(width) / static_cast<float>(height);

		const glm::mat4 projection =
			m_camera.GetProjectionMatrix(aspect);

		// Enable wireframe if selected
		glPolygonMode(
			GL_FRONT_AND_BACK,
			m_wireframe ? GL_LINE : GL_FILL
		);
		
		// Render the terrain
		// Set texture unit
		m_terrainShader->SetUniformInt("uAlbedo", 0);

		// Set sunlight direction
		m_terrainShader->setVec3(
			"uLightDirection",
			glm::vec3(0.5f, 1.0f, 0.3f)
		);

		// Bind rock texture
		m_rockMaterial.Bind(0);

		// Render all terrain chunks
		m_terrainManager.Render(
			*m_terrainShader,
			view,
			projection
		);

		// Unbind texture
		m_rockMaterial.Unbind(0);
		
		
		//m_terrainManager.Render(*m_terrainShader, view, projection);

		

		// Restore normal rendering for ImGui
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// ImGui
		m_window->BeginFrame();

		ImGui::Begin("ProjectWander Settings");

		ImGui::Text("Terrain Settings");

		ImGui::Separator();

		ImGui::Checkbox("Wireframe", &m_wireframe);

		ImGui::Separator();
		ImGui::SeparatorText("Terrain Streaming");

		ImGui::Text(
			"Loaded Chunks: %zu",
			m_terrainManager.GetChunkCount()
		);

		ImGui::Text(
			"Camera X: %.1f",
			m_camera.Position.x
		);

		ImGui::Text(
			"Camera Z: %.1f",
			m_camera.Position.z
		);

		ImGui::Separator();

		ImGui::Text("Camera");

		ImGui::Text(
			"Position: %.1f, %.1f, %.1f",
			m_camera.Position.x,
			m_camera.Position.y,
			m_camera.Position.z
		);

		ImGui::Text(
			"Yaw: %.1f  Pitch: %.1f",
			m_camera.Yaw,
			m_camera.Pitch
		);

		ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);

		ImGui::Text("W,A, S, D: Move camera");
		ImGui::Text("Space: Move up");
		ImGui::Text("Left Control: Move down");
		ImGui::Text("F1: Toggle camera/ImGui");
		ImGui::Text("Holding Shift should increase movement speed");



		ImGui::End();

		m_window->RenderImGui();

		glfwSwapBuffers(nativeWindow);
	}

	Shutdown();
	return 0;

}

void App::Shutdown()
{
	if (m_window)
	{
		// Release terrain resources
		m_terrainManager.Destroy();

		

		m_rockMaterial.Destroy();


		// Release shader resources
		m_terrainShader.reset();

		// Shutdown ImGui
		m_window->ImGuiShutdown();

		// Shutdown GLFW window
		m_window->Shutdown();
		m_window.reset();
	}

	m_isRunning = false;
}

void App::ProcessInput(GLFWwindow* window)
{
	// Escape closes the application
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, true);
	}

	// F1 toggles between camera and ImGui
	bool f1Down =
		glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;

	if (f1Down && !m_f1Pressed)
	{
		m_cameraActive = !m_cameraActive;

		glfwSetInputMode(
			window,
			GLFW_CURSOR,
			m_cameraActive ?
			GLFW_CURSOR_DISABLED :
			GLFW_CURSOR_NORMAL
		);

		// Reset mouse position tracking
		m_firstMouse = true;
	}

	m_f1Pressed = f1Down;

	// Don't move camera while using ImGui
	if (!m_cameraActive)
		return;

	// Hold Shift to move faster
	m_camera.MovementSpeed =
		glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
		? 40.0f
		: 15.0f;

	// Movement
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		m_camera.ProcessKeyboard(FORWARD, m_deltaTime);

	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		m_camera.ProcessKeyboard(BACKWARD, m_deltaTime);

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		m_camera.ProcessKeyboard(LEFT, m_deltaTime);

	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		m_camera.ProcessKeyboard(RIGHT, m_deltaTime);

	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
		m_camera.ProcessKeyboard(UP, m_deltaTime);

	if (glfwGetKey(window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
		m_camera.ProcessKeyboard(DOWN, m_deltaTime);
}

void App::ProcessMouse(GLFWwindow* window)
{
	if (!m_cameraActive)
		return;

	double mouseX;
	double mouseY;

	glfwGetCursorPos(window, &mouseX, &mouseY);

	if (m_firstMouse)
	{
		m_lastMouseX = mouseX;
		m_lastMouseY = mouseY;

		m_firstMouse = false;
		return;
	}

	float xOffset =
		static_cast<float>(mouseX - m_lastMouseX);

	float yOffset =
		static_cast<float>(m_lastMouseY - mouseY);

	m_lastMouseX = mouseX;
	m_lastMouseY = mouseY;

	m_camera.ProcessMouseMovement(
		xOffset,
		yOffset
	);
}