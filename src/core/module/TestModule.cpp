//
// Created by gabe on 6/22/25.
//

#include "TestModule.h"

namespace Engine {
	void TestModule::onInit()
	{
		log->info("Test module initialized.");
	}

	void TestModule::onUpdate(float dt)
	{
		log->debug("Updating frame! dt = {}", dt);
	}

	void TestModule::onShutdown()
	{
		log->info("Test module shutting down.");
	}
} // namespace Engine