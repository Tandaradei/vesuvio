#pragma once

#include "VulkanContext.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <string>

namespace vesuvio {

	class Application
	{
	public:
		Application(const std::string& appName);
		void run();


	private:
		void initWindow();
		void mainLoop();

		static void  framebufferResizeCallback(GLFWwindow* window, int width, int height) noexcept;
	private:
		const std::string appName;

		// window
		GLFWwindow* window;
		const uint32_t width = 800;
		const uint32_t height = 600;

		VulkanContext vkContext;


	};
}