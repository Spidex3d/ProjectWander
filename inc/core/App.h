#pragma once
#include <memory>

class Window;


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
	
	bool m_isRunning = false;	// Flag to indicate if the application is running

};