#pragma once

#include <stdint.h>
#include <optional>

//#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "Vertex.hpp"

#define VK_CHECK(expr) {vk::Result result = expr; assert(result == vk::Result::eSuccess);}


namespace vesuvio {

	vk::Result CreateDebugUtilsMessengerEXT(vk::Instance& instance, 
											const vk::DebugUtilsMessengerCreateInfoEXT& createInfo, 
											const vk::AllocationCallbacks* allocator, 
											vk::DebugUtilsMessengerEXT& debugMessenger);

	std::vector<char> readFile(const std::string& fileName);


	class Application
	{
	public:
		Application(const char* name);
		void run();


	private:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> graphics;
			std::optional<uint32_t> present;
			std::optional<uint32_t> transfer;

			bool isComplete() {
				return graphics.has_value() && present.has_value() && transfer.has_value();
			}
		};

		struct SwapChainSupportDetails
		{
			vk::SurfaceCapabilitiesKHR capabilities;
			std::vector<vk::SurfaceFormatKHR> formats;
			std::vector<vk::PresentModeKHR> presentModes;
		};

		struct AllocatedBuffer
		{
			vk::Buffer buffer;
			vk::DeviceMemory memory;
		};
	private:
		void initWindow();
		void initVulkan();
		void mainLoop();
		void cleanup();

		void createInstance();
		void setupDebugMessenger();
		void createSurface();
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createSwapChain();
		void createSwapChainImageViews();
		void createSyncObjects();
		void createRenderPass();
		void createDescriptorSetLayout();
		void createGraphicsPipeline();
		void createFramebuffers();
		void createCommandPools();
		void createDepthResources();
		void createTextureImage();
		void createTextureImageView();
		void createTextureSampler();
		void createVertexBuffer();
		void createIndexBuffer();
		void createUniformBuffers();
		void createDescriptorPool();
		void createDescriptorSets();
		void createCommandBuffers();

		void cleanupSwapChain();
		void recreateSwapChain();

		void drawFrame();
		void updateUniformBuffer(uint32_t currentImage);

		float rateDevice(const vk::PhysicalDevice& device);
		bool checkDeviceExtensionSupport(const vk::PhysicalDevice& device);
		QueueFamilyIndices findQueueFamilies(const vk::PhysicalDevice& device);
		SwapChainSupportDetails querySwapChainSupport(const vk::PhysicalDevice& device);
		vk::SurfaceFormatKHR chooseSwapChainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
		vk::PresentModeKHR chooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
		vk::Extent2D chooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
		vk::DebugUtilsMessengerCreateInfoEXT getDebugMessengerCreateInfo();
		vk::ShaderModule createShaderModule(const std::vector<char>& shaderCode);
		vk::ImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);

		uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);
		AllocatedBuffer createAllocatedBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
											  vk::ArrayProxy<uint32_t> queueFamilyIndices, vk::MemoryPropertyFlags properties);
		void createImage2D(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::MemoryPropertyFlags memoryProperties, vk::Image& dstImage, vk::DeviceMemory& dstImageMemory);
		void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandBuffer commandBuffer = nullptr);
		void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, vk::CommandBuffer commandBuffer = nullptr);
		vk::CommandBuffer beginOneTimeCommandBuffer(vk::CommandPool pool);
		void endOneTimeCommandBuffer(vk::CommandBuffer commandBuffer, vk::CommandPool pool, vk::Queue queue);
		void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer commandBuffer = nullptr);
		
		bool checkValidationLayerSupport();
		std::vector<const char*> getRequiredExtensions();

		vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features);
		vk::Format findDepthFormat();
		bool hasStencilComponent(vk::Format format);

		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
															VkDebugUtilsMessageTypeFlagsEXT messageType,
															const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
															void* pUserData);

		static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

		

	private:
		// application
		const char* appName;
		uint32_t currentFrame;
		// window
		GLFWwindow* window;
		const uint32_t width = 800;
		const uint32_t height = 600;
		const char* windowName = "vesuvio";
		bool framebufferResized = false;
		// vulkan
		vk::Instance instance;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		vk::Queue graphicsQueue;
		vk::Queue presentQueue;
		vk::Queue transferQueue;
		vk::SwapchainKHR swapChain;
		std::vector<vk::Image> swapChainImages;
		vk::Format swapChainFormat;
		vk::Extent2D swapChainExtent;
		std::vector<vk::ImageView> swapChainImageViews;
		std::vector<vk::Framebuffer> swapChainFramebuffers;
		const uint32_t MAX_FRAMES_IN_FLIGHT = 2;
		vk::RenderPass renderPass;
		vk::CommandPool graphicsCommandPool;
		vk::CommandPool transferCommandPool;
		std::vector<vk::CommandBuffer> commandBuffers;
		std::vector<vk::Semaphore> imageAvailableSemaphores;
		std::vector<vk::Semaphore> renderFinishedSemaphores;
		std::vector<vk::Fence> inFlightFences;
		std::vector<vk::Fence> imagesInFlight;

		vk::Fence bufferCopyFence;

		QueueFamilyIndices queueFamilyIndices;

		vk::DescriptorSetLayout descriptorSetLayout;
		vk::DescriptorPool descriptorPool;
		std::vector<vk::DescriptorSet> descriptorSets;
		vk::PipelineLayout pipelineLayout;
		vk::Pipeline graphicsPipeline;

		vk::Image depthImage;
		vk::DeviceMemory depthImageMemory;
		vk::ImageView depthImageView;

		vk::Image textureImage;
		vk::DeviceMemory textureImageMemory;
		vk::ImageView textureImageView;
		vk::Sampler textureSampler;

		AllocatedBuffer vertexBuffer;
		AllocatedBuffer indexBuffer;
		std::vector<AllocatedBuffer> uniformBuffers;

		const std::vector<const char*> deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};

		const std::vector<Vertex> vertices = {
			{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

			{{-0.5f, -1.0f, 0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
			{{0.5f, -1.0f, 0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
			{{0.5f, 0.0f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
			{{-0.5f, 0.0f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
		};

		const std::vector<uint16_t> indices = {
			0, 1, 2, 2, 3, 0,
			4, 5, 6, 6, 7, 4
		};

		// vulkan debug
		const std::vector<const char*> validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		vk::DebugUtilsMessengerEXT debugMessenger;

		#ifdef NDEBUG
			const bool enableValidationLayers = false;
		#else
			const bool enableValidationLayers = true;
		#endif
	};
}