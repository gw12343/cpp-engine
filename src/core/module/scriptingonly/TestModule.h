//
// Created by gabe on 6/22/25.
//
#pragma once

#include "core/module/Module.h"

namespace Engine {

	class TestModule : public Module {
	  public:
		void                      onInit() override;
		void                      onUpdate(float dt) override;
		void                      onShutdown() override;
		[[nodiscard]] std::string name() const override { return "TestModule"; }
	};

} // namespace Engine