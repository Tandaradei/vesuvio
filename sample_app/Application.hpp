#pragma once

#include "vesuvio/runtime/Runtime.hpp"

#include <string>

namespace sample {

	struct vesuvio::GfxContext;

	class Application
	{
	public:
		Application(const std::string& appName);
		~Application();
		void run();


	public:
		void update(float frametime);
	private:
		const std::string appName;
		const uint16_t width = 800;
		const uint16_t height = 600;

		vesuvio::Runtime runtime;
	};
}