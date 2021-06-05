#include "Application.hpp"

namespace vesuvio {
	Application::Application(const std::string& appName)
	: appName(appName)
	, window()
	, vkContext()
	{

	}
	void Application::run() {
		initWindow();
		vkContext.initVulkan(appName, window);
		mainLoop();
		vkContext.cleanup();
	}
	void Application::initWindow() {
		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		window = glfwCreateWindow((int)width, (int)height, appName.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, &vkContext);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
	}

	void Application::mainLoop() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			vkContext.mainLoop();
		}
	}

	void Application::framebufferResizeCallback(GLFWwindow* window, int width, int height) noexcept {
		Application* app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
		VulkanContext::framebufferResizeCallback(&(app->vkContext), width, height);
	}
}