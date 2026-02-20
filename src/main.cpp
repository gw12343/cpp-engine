#include "core/Engine.h"


using namespace Engine;

int main()
{
	spdlog::info("Running in {}", std::filesystem::current_path().string());
	GEngine engine(1600, 1200, "cpp-engine");

	if (!engine.Initialize()) {
		spdlog::critical("Failed to init engine");
		return -1;
	}

	engine.Run();
	engine.Shutdown();
	return 0;
}
