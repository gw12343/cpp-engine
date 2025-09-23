#include <core/Engine.h>
using namespace Engine;

#include <iostream>

int main()
{
	std::cout << "Hello World!" << std::endl;

	GEngine engine(1600, 1200, "cpp-engine");
	//
	//	if (!engine.Initialize()) {
	//		spdlog::critical("Failed to init engine");
	//		return -1;
	//	}
	//
	//	engine.Run();
	//	engine.Shutdown();
	return 0;
}
