#include <core/Engine.h>
using namespace Engine;

int main()
{
	GEngine engine(1600 * 2, 1200 * 2, "Hello World");

	if (!engine.Initialize()) {
		return -1;
	}

	engine.Run();
	engine.Shutdown();
	return 0;
}
