#pragma once
#include <memory>
//#include <GLFW/glfw3.h>
#include <camera\Camera.h>
#include <world/Terrain.h>
#include <graphics/Shader.h>
#include "world/TerrainManager.h"
#include "graphics/Material.h"

class Window;
struct GLFWwindow;

class App
{

public:
	App();
	~App();

	bool Init(); // Initialize the window and ImGui context
	int Run();        // set the main loop





	void Shutdown();

private:

	std::unique_ptr<Window> m_window;			// Window instance for managing the application window
	Camera m_camera;

	// Camera controls
	bool m_cameraActive = true;
	bool m_firstMouse = true;

	double m_lastMouseX = 0.0;
	double m_lastMouseY = 0.0;

	// Frame timing
	float m_deltaTime = 0.0f;
	float m_lastFrame = 0.0f;

	// Input state
	bool m_f1Pressed = false;

	// Camera input functions
	void ProcessInput(GLFWwindow* window);
	void ProcessMouse(GLFWwindow* window);


	
	TerrainManager m_terrainManager;

	std::unique_ptr<Shader> m_terrainShader;
	bool m_terrainGenerated = false;	// Flag to indicate if the terrain has been generated
	bool m_terrainRendered = false;	// Flag to indicate if the terrain has been rendered
	
	Material m_rockMaterial;

	bool m_wireframe = false;	// Flag to indicate if wireframe mode is enabled


	bool m_isRunning = false;	// Flag to indicate if the application is running

};