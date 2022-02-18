#include "Application.hpp"
#include "vesuvio/gfx/GfxContext.hpp"

namespace sample {

	Application::Application(const std::string& appName)
	: appName(appName)
	{
		vesuvio::Runtime::WindowInit window;
		window.name = appName.c_str();
		window.width = width;
		window.height = height;
		vesuvio::Runtime::GfxInit gfx;
		gfx.gfxBackend = vesuvio::GfxContext::GfxBackend::Vulkan;
		vesuvio::Runtime::UpdateFunc updateFn = [&](float ft) { update(ft); };
		runtime.init(window, gfx, updateFn);
	}

	Application::~Application() {
	}

	void Application::run() {
		runtime.run();
	}

	void Application::update(float frametime) {

	}

	
}