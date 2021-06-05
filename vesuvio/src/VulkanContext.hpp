#pragma once

#include <stdint.h>
#include <optional>
#include <string>

#include <vulkan/vulkan.hpp>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "vk_mem_alloc.h"

#include "Vertex.hpp"

#define VK_CHECK(expr) {vk::Result result = expr; assert(result == vk::Result::eSuccess);}


namespace vesuvio {

	vk::Result CreateDebugUtilsMessengerEXT(vk::Instance& instance, 
											const vk::DebugUtilsMessengerCreateInfoEXT& createInfo, 
											const vk::AllocationCallbacks* allocator, 
											vk::DebugUtilsMessengerEXT& debugMessenger);

	std::vector<char> readFile(const std::string& fileName);


	class VulkanContext
	{
	public:
		VulkanContext();
		void initVulkan(const std::string& appName, GLFWwindow* window);
		void mainLoop();
		void cleanup();

		static void framebufferResizeCallback(VulkanContext* vkContext, int width, int height) noexcept;

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

	private:

		void createInstance(const std::string& appName);
		void setupDebugMessenger();
		void createSurface(GLFWwindow* window);
		void pickPhysicalDevice();
		void createLogicalDevice();
		void createVmaAllocator();
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
		vk::Result createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage,
								vk::ArrayProxy<uint32_t> queueFamilyIndices, VmaMemoryUsage memoryUsage,
								vk::Buffer& buffer, VmaAllocation& allocation);
		vk::Result createImage2D(uint32_t width, uint32_t height, 
								vk::Format format, vk::ImageTiling tiling, 
								vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage,
								vk::Image& image, VmaAllocation& allocation);
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

	private:
		// application
		uint32_t currentFrame;
		// window
		GLFWwindow* window;
		bool framebufferResized = false;
		// vulkan
		vk::Instance instance;
		vk::SurfaceKHR surface;
		vk::PhysicalDevice physicalDevice;
		vk::Device device;
		VmaAllocator allocator;
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
		VmaAllocation depthImageAlloc;
		vk::ImageView depthImageView;

		vk::Image textureImage;
		VmaAllocation textureImageAlloc;
		vk::ImageView textureImageView;
		vk::Sampler textureSampler;

		vk::Buffer vertexBuffer;
		VmaAllocation vertexBufferAlloc;
		vk::Buffer indexBuffer;
		VmaAllocation indexBufferAlloc;
		std::vector<vk::Buffer> uniformBuffers;
		std::vector<VmaAllocation> uniformBufferAllocs;

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