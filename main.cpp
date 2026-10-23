#include "core/App.h"
#include <miniBoxLog.h>

int main() {
	App app;

	if (!app.Init()) {
		BOX_LOG_DEBUG("Failed to initialize Project Wander");
		return -1;
	}

	BOX_LOG_INFO("Starting Project Wander application...");

	return app.Run();
}