#include "core/App.h"
#include <miniBoxLog.h>
#include <GLFW/glfw3.h>	
#include <core/Window.h>
#include <imgui/imgui.h>

#include <random>
#include <limits>

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
		"assets/materials/rock/Blue_Rock.mat"))
	{
		BOX_LOG_ERROR("Failed to load Blue_Rock material");
		return false;
	}
	if (!m_grassMaterial.Load(
		"assets/materials/grass/Green_grass.mat"))
	{
		BOX_LOG_ERROR("Failed to load Green_grass material");
		return false;
	}

	if (!m_terrainManager.Initialize())
	{
		BOX_LOG_DEBUG("Failed to initialize terrain chunks");
		return false;
	}

	m_pendingTerrainSettings = m_terrainManager.GetSettings();

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

		// Keep camera on the ground in walking mode
		if (m_walkMode)
		{
			float groundHeight = 0.0f;

			if (m_terrainManager.GetHeightAt(
				m_camera.Position.x,
				m_camera.Position.z,
				groundHeight))
			{
				m_camera.Position.y =
					groundHeight + m_eyeHeight;
			}
		}

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
		

		// Activate terrain shader
		m_terrainShader->Use();

		// Normal map texture units
		m_terrainShader->SetUniformInt(
			"uGrassNormal", 2
		);

		m_terrainShader->SetUniformInt(
			"uRockNormal", 3
		);

		// Normal map strength
		m_terrainShader->SetUniformFloat(
			"uGrassNormalStrength",
			m_grassNormalStrength
		);

		m_terrainShader->SetUniformFloat(
			"uRockNormalStrength",
			m_rockNormalStrength
		);



		// Triplanar texture settings
		m_terrainShader->SetUniformFloat(
			"uTextureScale",
			m_textureScale
		);

		m_terrainShader->SetUniformFloat(
			"uTriplanarSharpness",
			m_triplanarSharpness
		);

		m_terrainShader->SetUniformFloat(
			"uRockStart",
			m_rockStart
		);

		m_terrainShader->SetUniformFloat(
			"uRockEnd",
			m_rockEnd
		);

		m_terrainShader->SetUniformFloat(
			"uBlendNoise",
			m_blendNoise
		);

		// Assign texture units
		m_terrainShader->SetUniformInt(
			"uGrassAlbedo", 0
		);

		m_terrainShader->SetUniformInt(
			"uRockAlbedo", 1
		);

		// Set sunlight direction
		m_terrainShader->setVec3(
			"uLightDirection",
			glm::vec3(0.5f, 1.0f, 0.3f)
		);

		// Bind both terrain materials
		// Colour textures
		m_grassMaterial.Bind(0);
		m_rockMaterial.Bind(1);

		// Normal maps
		m_grassMaterial.BindNormal(2);
		m_rockMaterial.BindNormal(3);

		// Render all terrain chunks
		m_terrainManager.Render(
			*m_terrainShader,
			view,
			projection
		);



		// Render terrain chunks
		m_terrainManager.Render(
			*m_terrainShader,
			view,
			projection
		);

		// Unbind materials
		m_grassMaterial.Unbind(0);
		m_rockMaterial.Unbind(1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, 0);

		// Return to the default texture unit
		glActiveTexture(GL_TEXTURE0);

		

		// Restore normal rendering for ImGui
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		// ImGui
		m_window->BeginFrame();

		ImGui::Begin("ProjectWander Settings");

		ImGui::Text("Terrain Settings");

		ImGui::Separator();

		ImGui::Checkbox("Wireframe", &m_wireframe);

		ImGui::SeparatorText("World Generation");

		ImGui::InputInt(
			"World Seed",
			&m_pendingTerrainSettings.WorldSeed
		);

		// Keep the seed non-negative
		if (m_pendingTerrainSettings.WorldSeed < 0)
		{
			m_pendingTerrainSettings.WorldSeed = 0;
		}
		if (ImGui::Button("Random Seed"))
		{
			static std::mt19937 generator(
				std::random_device{}()
			);

			static std::uniform_int_distribution<int> distribution(
				0,
				std::numeric_limits<int>::max()
			);

			m_pendingTerrainSettings.WorldSeed =
				distribution(generator);
		}

		ImGui::SeparatorText("Terrain Generator");

		// ====================================
		// Rolling hills
		// ====================================

		ImGui::Text("Rolling Hills");

		ImGui::SliderFloat(
			"Hill Height",
			&m_pendingTerrainSettings.HillHeight,
			0.0f,
			100.0f
		);

		ImGui::SliderFloat(
			"Noise Frequency",
			&m_pendingTerrainSettings.NoiseFrequency,
			0.001f,
			0.05f,
			"%.4f"
		);

		ImGui::SliderInt(
			"Noise Octaves",
			&m_pendingTerrainSettings.NoiseOctaves,
			1,
			8
		);


		// ====================================
		// Mountains
		// ====================================

		ImGui::SeparatorText("Mountains");

		ImGui::SliderFloat(
			"Mountain Height",
			&m_pendingTerrainSettings.MountainHeight,
			0.0f,
			10.0f
		);

		ImGui::SliderFloat(
			"Ridge Sharpness",
			&m_pendingTerrainSettings.RidgeSharpness,
			1.0f,
			8.0f
		);

		ImGui::SliderFloat(
			"Mountain Frequency",
			&m_pendingTerrainSettings.MountainFrequency,
			0.0001f,
			0.01f,
			"%.4f"
		);


		// ====================================
		// Mountain coverage
		// ====================================

		ImGui::SeparatorText("Mountain Coverage");

		ImGui::SliderFloat(
			"Mountain Start",
			&m_pendingTerrainSettings.MountainStart,
			0.0f,
			1.0f
		);

		ImGui::SliderFloat(
			"Mountain End",
			&m_pendingTerrainSettings.MountainEnd,
			0.0f,
			1.0f
		);

		// Keep the blending range valid
		if (m_pendingTerrainSettings.MountainEnd <=
			m_pendingTerrainSettings.MountainStart)
		{
			m_pendingTerrainSettings.MountainEnd =
				m_pendingTerrainSettings.MountainStart + 0.01f;
		}


		// ====================================
		// Regenerate
		// ====================================

		ImGui::Separator();

		if (ImGui::Button("Regenerate Terrain"))
		{
			m_terrainManager.Regenerate(
				m_pendingTerrainSettings,
				m_camera.Position
			);
		}



		ImGui::SeparatorText("Triplanar Mapping");

		ImGui::SliderFloat(
			"Texture Scale",
			&m_textureScale,
			0.01f,
			0.5f,
			"%.3f"
		);

		ImGui::SliderFloat(
			"Projection Sharpness",
			&m_triplanarSharpness,
			1.0f,
			16.0f
		);

		ImGui::SeparatorText("Terrain Materials");

		ImGui::SliderFloat("Rock Start",
			&m_rockStart, 0.0f, 0.5f);

		ImGui::SliderFloat("Rock End",
			&m_rockEnd, 0.0f, 0.8f);

		ImGui::SliderFloat("Blend Variation",
			&m_blendNoise, 0.0f, 0.2f);

		// Prevent invalid blending range
		if (m_rockEnd <= m_rockStart)
		{
			m_rockEnd = m_rockStart + 0.01f;
		}

		ImGui::SeparatorText("Normal Mapping");

		ImGui::SliderFloat(
			"Grass Normal Strength",
			&m_grassNormalStrength,
			0.0f,
			2.0f
		);

		ImGui::SliderFloat(
			"Rock Normal Strength",
			&m_rockNormalStrength,
			0.0f,
			2.0f
		);

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

		ImGui::SeparatorText("Camera Movement");

		ImGui::Checkbox(
			"Walking Mode",
			&m_walkMode
		);

		if (m_walkMode)
		{
			ImGui::SliderFloat(
				"Eye Height",
				&m_eyeHeight,
				0.5f,
				3.0f
			);

			ImGui::SliderFloat(
				"Walk Speed",
				&m_walkSpeed,
				1.0f,
				15.0f
			);
		}

		ImGui::Text(
			"Camera Y: %.2f",
			m_camera.Position.y
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

		
		m_grassMaterial.Destroy();
		m_rockMaterial.Destroy();

		// Release terrain material resources
		
		
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
	bool f1Down = glfwGetKey(window, GLFW_KEY_F1) == GLFW_PRESS;

	if (f1Down && !m_f1Pressed)
	{
		m_cameraActive = !m_cameraActive;

		glfwSetInputMode(window, GLFW_CURSOR, m_cameraActive ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

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



	// Movement speed
	float speedMultiplier =
		glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
		? 2.0f
		: 1.0f;

	if (m_walkMode)
	{
		// Walking movement stays horizontal
		glm::vec3 forward = m_camera.Front;
		forward.y = 0.0f;

		if (glm::length(forward) > 0.001f)
			forward = glm::normalize(forward);

		glm::vec3 right = m_camera.Right;
		right.y = 0.0f;

		if (glm::length(right) > 0.001f)
			right = glm::normalize(right);

		glm::vec3 movement(0.0f);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			movement += forward;

		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			movement -= forward;

		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			movement += right;

		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			movement -= right;

		// Normalise to avoid faster diagonal movement
		if (glm::length(movement) > 0.001f)
		{
			movement = glm::normalize(movement);

			m_camera.Position +=
				movement *
				m_walkSpeed *
				speedMultiplier *
				m_deltaTime;
		}
	}
	else
	{
		// Original free-fly movement
		m_camera.MovementSpeed =
			15.0f * speedMultiplier;

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