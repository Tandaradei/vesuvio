#include "VulkanContext.hpp"

#include <fstream>
#include <chrono>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>


#include "UniformBufferObject.hpp"
#include "VertexBuffer.hpp"

namespace vesuvio {

	vk::Result CreateDebugUtilsMessengerEXT(vk::Instance& instance, const vk::DebugUtilsMessengerCreateInfoEXT& createInfo, const vk::AllocationCallbacks* allocator, vk::DebugUtilsMessengerEXT& debugMessenger) {
		PFN_vkCreateDebugUtilsMessengerEXT func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));
		if (func != nullptr) {
			VkResult result = func(instance, (VkDebugUtilsMessengerCreateInfoEXT*)(&createInfo), (VkAllocationCallbacks*)(allocator), (VkDebugUtilsMessengerEXT*)(&debugMessenger));
			return static_cast<vk::Result>(result);
		}
		else {
			return vk::Result::eErrorExtensionNotPresent;
		}
	}

	std::vector<char> readFile(const std::string& fileName) {
		std::ifstream file(fileName, std::ios::ate | std::ios::binary);

		assert(file.is_open());
		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);
		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		printf("Read '%s' (%lu bytes)\n", fileName.data(), buffer.size());
		return buffer;
	}

	void DestroyDebugUtilsMessengerEXT(vk::Instance& instance, vk::DebugUtilsMessengerEXT debugMessenger, const vk::AllocationCallbacks* allocator) {
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, (VkAllocationCallbacks*)allocator);
		}
	}

	VulkanContext::VulkanContext()
	: instance()
	, allocator()
	, currentFrame(0)
	, depthImageAlloc()
	, swapChainFormat()
	, window(nullptr)
	{

	}

	void VulkanContext::init(const char* appName, GLFWwindow* window) {
		this->window = window;
		createInstance(appName);
		setupDebugMessenger();
		createSurface(window);
		pickPhysicalDevice();
		createLogicalDevice();
		createVmaAllocator();
		createSwapChain();
		createSwapChainImageViews();
		createSyncObjects();
		createRenderPass();
		createDescriptorSetLayout();
		createGraphicsPipeline();
		createCommandPools();
		createDepthResources();
		createFramebuffers();
		createSampledImage();
		createTextureSampler();
		vb = createVertexBuffer(vertices.data(), (uint16_t)vertices.size());
		ib = createIndexBuffer(indices.data(), (uint32_t)indices.size());
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
	}

	void VulkanContext::createInstance(const char* appName) {

		vk::ApplicationInfo appInfo(
			appName,
			VK_MAKE_VERSION(0, 0, 1),
			"vesuvio",
			VK_MAKE_VERSION(0, 0, 1),
			VK_API_VERSION_1_2
		);

		auto supportedExtensions = vk::enumerateInstanceExtensionProperties(nullptr);

		printf("%zu extensions supported:\n", supportedExtensions.size());
		for (const auto& extension : supportedExtensions) {
			printf("     %s\n", extension.extensionName.data());
		}

		std::vector<const char*> requiredExtensions = getRequiredExtensions();

		printf("%u extensions requested:\n", static_cast<uint32_t>(requiredExtensions.size()));
		for (const auto requiredExtension : requiredExtensions) {
			bool isSupported = false;
			for (const auto& extension : supportedExtensions) {
				if (!strcmp(requiredExtension, extension.extensionName)) {
					isSupported = true;
					break;
				}
			}
			printf(" [%c] %s\n", isSupported ? 'x' : ' ', requiredExtension);
		}

		vk::InstanceCreateInfo createInfo(
			{},
			&appInfo,
			0,
			nullptr,
			static_cast<uint32_t>(requiredExtensions.size()),
			requiredExtensions.data()
		);
		vk::DebugUtilsMessengerCreateInfoEXT debugCreateInfo;
		if (enableValidationLayers) {
			assert(checkValidationLayerSupport() && "validation layers requested, but not available");
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			debugCreateInfo = getDebugMessengerCreateInfo();
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		}
		else {
			createInfo.enabledLayerCount = 0;
		}

		VK_CHECK(vk::createInstance(&createInfo, nullptr, &instance))
	}

	vk::DebugUtilsMessengerCreateInfoEXT VulkanContext::getDebugMessengerCreateInfo() {
		return vk::DebugUtilsMessengerCreateInfoEXT(
			vk::DebugUtilsMessengerCreateFlagsEXT(0),
			vk::DebugUtilsMessageSeverityFlagsEXT(/*VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |*/ VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT),
			vk::DebugUtilsMessageTypeFlagsEXT(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT),
			debugCallback,
			nullptr
		);
	}

	vk::ShaderModule VulkanContext::createShaderModule(const std::vector<char>& shaderCode) {
		vk::ShaderModuleCreateInfo createInfo{};
		createInfo.codeSize = shaderCode.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

		vk::ShaderModule shaderModule = device.createShaderModule(createInfo);
		assert(shaderModule);
		return shaderModule;
	}

	vk::ImageView VulkanContext::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
		vk::ImageViewCreateInfo viewInfo{};
		viewInfo.image = image;
		viewInfo.viewType = vk::ImageViewType::e2D;
		viewInfo.format = format;
		viewInfo.components.r = vk::ComponentSwizzle::eIdentity;
		viewInfo.components.g = vk::ComponentSwizzle::eIdentity;
		viewInfo.components.b = vk::ComponentSwizzle::eIdentity;
		viewInfo.components.a = vk::ComponentSwizzle::eIdentity;
		viewInfo.subresourceRange.aspectMask = aspectFlags;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		vk::ImageView imageView = device.createImageView(viewInfo);
		assert(imageView);

		return imageView;
	}

	uint32_t VulkanContext::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
		vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

		for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
			if (typeFilter & (1 << i) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		assert(false && "no suitable memory type found!");
		return 0;
	}

	vk::Result VulkanContext::createBuffer(vk::DeviceSize size, vk::BufferUsageFlags usage, vk::ArrayProxy<uint32_t> queueFamilyIndices, VmaMemoryUsage memoryUsage, vk::Buffer& buffer, VmaAllocation& allocation) {
		vk::BufferCreateInfo bufferInfo{};
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.queueFamilyIndexCount = queueFamilyIndices.size();
		bufferInfo.pQueueFamilyIndices = queueFamilyIndices.data();
		bufferInfo.sharingMode = queueFamilyIndices.size() > 1 ? vk::SharingMode::eConcurrent : vk::SharingMode::eExclusive;

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;
		VkResult result = vmaCreateBuffer(allocator, (VkBufferCreateInfo*)&bufferInfo, &allocInfo, (VkBuffer*)&buffer, &allocation, nullptr);
		return vk::Result(result);
	}

	vk::Result VulkanContext::createImage2D(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, VmaMemoryUsage memoryUsage, vk::Image& image, VmaAllocation& allocation) {
		vk::ImageCreateInfo imageInfo{};
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling;
		imageInfo.usage = usage;
		imageInfo.sharingMode = vk::SharingMode::eExclusive;
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.flags = vk::ImageCreateFlags(); // Optional

		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = memoryUsage;

		VkResult result = vmaCreateImage(allocator, (VkImageCreateInfo*)&imageInfo, &allocInfo, (VkImage*)&image, &allocation, nullptr);
		return vk::Result(result);
	}

	void VulkanContext::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size, vk::CommandBuffer commandBuffer) {
		bool usesLocalCommandBuffer = !commandBuffer;
		if (usesLocalCommandBuffer) {
			commandBuffer = beginOneTimeCommandBuffer(transferCommandPool);
		}

		vk::BufferCopy copyRegion{};
		copyRegion.srcOffset = 0;
		copyRegion.dstOffset = 0;
		copyRegion.size = size;
		commandBuffer.copyBuffer(srcBuffer, dstBuffer, copyRegion);

		if (usesLocalCommandBuffer) {
			endOneTimeCommandBuffer(commandBuffer, transferCommandPool, transferQueue);
		}

		printf("Copied %llu bytes between buffers\n", size);
	}

	void VulkanContext::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, vk::CommandBuffer commandBuffer) {
		bool usesLocalCommandBuffer = !commandBuffer;
		if (usesLocalCommandBuffer) {
			commandBuffer = beginOneTimeCommandBuffer(transferCommandPool);
		}

		vk::BufferImageCopy copyRegion{};
		copyRegion.bufferOffset = 0;
		copyRegion.bufferRowLength = 0;
		copyRegion.bufferImageHeight = 0;

		copyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
		copyRegion.imageSubresource.mipLevel = 0;
		copyRegion.imageSubresource.baseArrayLayer = 0;
		copyRegion.imageSubresource.layerCount = 1;

		copyRegion.imageOffset = vk::Offset3D{ 0, 0, 0 };
		copyRegion.imageExtent = vk::Extent3D{ width, height, 1 };

		commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, copyRegion);

		if (usesLocalCommandBuffer) {
			endOneTimeCommandBuffer(commandBuffer, transferCommandPool, transferQueue);
		}

		printf("Copied from buffer to image (%ux%u)\n", width, height);
	}

	vk::CommandBuffer VulkanContext::beginOneTimeCommandBuffer(vk::CommandPool pool) {
		vk::CommandBufferAllocateInfo allocInfo{};
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandPool = pool;
		allocInfo.commandBufferCount = 1;

		vk::CommandBuffer commandBuffer = device.allocateCommandBuffers(allocInfo)[0];

		vk::CommandBufferBeginInfo beginInfo{};
		beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

		commandBuffer.begin(beginInfo);

		return commandBuffer;
	}

	void VulkanContext::endOneTimeCommandBuffer(vk::CommandBuffer commandBuffer, vk::CommandPool pool, vk::Queue queue) {
		commandBuffer.end();

		vk::SubmitInfo submitInfo{};
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		queue.submit(submitInfo, nullptr);
		queue.waitIdle();

		device.freeCommandBuffers(pool, commandBuffer);
	}

	void VulkanContext::transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::CommandBuffer commandBuffer) {
		bool usesLocalCommandBuffer = !commandBuffer;
		if (usesLocalCommandBuffer) {
			commandBuffer = beginOneTimeCommandBuffer(graphicsCommandPool);
		}

		vk::ImageMemoryBarrier imageBarrier{};
		imageBarrier.oldLayout = oldLayout;
		imageBarrier.newLayout = newLayout;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.image = image;
		if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;

			if (hasStencilComponent(format)) {
				imageBarrier.subresourceRange.aspectMask |= vk::ImageAspectFlagBits::eStencil;
			}
		}
		else {
			imageBarrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
		}
		imageBarrier.subresourceRange.baseMipLevel = 0;
		imageBarrier.subresourceRange.levelCount = 1;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 1;

		imageBarrier.srcAccessMask = vk::AccessFlags(); // TODO
		imageBarrier.dstAccessMask = vk::AccessFlags(); // TODO

		vk::PipelineStageFlags sourceStage;
		vk::PipelineStageFlags destinationStage;

		if (oldLayout == vk::ImageLayout::eUndefined ) {
			imageBarrier.srcAccessMask = vk::AccessFlags();
			sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
		}
		else if (oldLayout == vk::ImageLayout::eTransferDstOptimal) {
			imageBarrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			sourceStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else {
			assert(false);
		}

		if (newLayout == vk::ImageLayout::eTransferDstOptimal) {
			imageBarrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
			destinationStage = vk::PipelineStageFlagBits::eTransfer;
		}
		else if (newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			imageBarrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
			destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
		}
		else if (newLayout == vk::ImageLayout::eDepthStencilAttachmentOptimal) {
			imageBarrier.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;
			destinationStage = vk::PipelineStageFlagBits::eEarlyFragmentTests;
		}
		else {
			assert(false);
		}

		commandBuffer.pipelineBarrier(
			sourceStage, destinationStage,
			vk::DependencyFlags(),
			{}, // No memory barriers
			{}, // No buffer memory barriers
			imageBarrier
		);

		if (usesLocalCommandBuffer) {
			endOneTimeCommandBuffer(commandBuffer, graphicsCommandPool, graphicsQueue);
		}
	}

	void VulkanContext::setupDebugMessenger() {
		if (!enableValidationLayers) return;

		vk::DebugUtilsMessengerCreateInfoEXT createInfo = getDebugMessengerCreateInfo();

		VK_CHECK(CreateDebugUtilsMessengerEXT(instance, createInfo, nullptr, debugMessenger))
	}

	void VulkanContext::createSurface(GLFWwindow* glfwWindow) {
		VK_CHECK(vk::Result(glfwCreateWindowSurface(instance, glfwWindow, nullptr, (VkSurfaceKHR*)(&surface))))

	}

	void VulkanContext::pickPhysicalDevice() {
		auto devices = instance.enumeratePhysicalDevices();

		assert(devices.size() > 0);
		float bestRating = 0.0f;

		for (const auto& device : devices) {
			float rating = rateDevice(device);
			if (rating > bestRating) {
				physicalDevice = device;
				bestRating = rating;
			}
		}

		assert(bestRating > 0.0f);
		printf("Picked physical device: %s\n", physicalDevice.getProperties().deviceName.data());
	}

	float VulkanContext::rateDevice(const vk::PhysicalDevice& device) {
		printf("Rating device %s\n", device.getProperties().deviceName.data());

		float rating = 1.0f;
		if (!findQueueFamilies(device).isComplete()) {
			printf("Didn't find all required queue families!\n");
			rating = 0.0f;
		}
		if (checkDeviceExtensionSupport(device)) {
			auto swapChainSupport = querySwapChainSupport(device);
			if (swapChainSupport.formats.empty()) {
				printf("No swap chain formats found!\n");
				rating = 0.0f;
			}
			if (swapChainSupport.presentModes.empty()) {
				printf("No swap chain present modes found!\n");
				rating = 0.0f;
			}
		}
		else {
			rating = 0.0f;
		}

		if (rating > 0.0f) {
			vk::PhysicalDeviceProperties properties = device.getProperties();
			vk::PhysicalDeviceFeatures features = device.getFeatures();

			if (properties.deviceType == vk::PhysicalDeviceType::eDiscreteGpu) {
				rating += 1000.0f;
			}
		}
		printf("Rating: %.0f\n", rating);
		return rating;
	}

	bool VulkanContext::checkDeviceExtensionSupport(const vk::PhysicalDevice& device) {
		printf("Checking for device extensions:\n");
		auto availableExtensions = device.enumerateDeviceExtensionProperties();
		uint32_t foundExtensions = 0;
		for (const auto& neededExtension : deviceExtensions) {
			bool found = false;
			for (const auto& availableExtension : availableExtensions) {
				if (!strcmp(neededExtension, availableExtension.extensionName.data())) {
					found = true;
					foundExtensions++;
					break;
				}
			}
			printf(" [%s] %s\n", found ? "x" : " ", neededExtension);
		}
		return foundExtensions == deviceExtensions.size();
	}

	VulkanContext::QueueFamilyIndices VulkanContext::findQueueFamilies(const vk::PhysicalDevice& device) {
		QueueFamilyIndices indices;

		auto queueFamilies = device.getQueueFamilyProperties();

		uint32_t i = 0;
		for (const auto& queueFamily : queueFamilies) {
			if (!indices.graphics.has_value() && queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
				indices.graphics = i;
			}
			if (!indices.present.has_value() && device.getSurfaceSupportKHR(i, surface)) {
				indices.present = i;
			}
			if (!indices.transfer.has_value() && queueFamily.queueFlags & vk::QueueFlagBits::eTransfer && !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
				indices.transfer = i;
			}
			// early exit if already complete
			if (indices.isComplete()) {
				break;
			}
			i++;
		}

		return indices;
	}

	VulkanContext::SwapChainSupportDetails VulkanContext::querySwapChainSupport(const vk::PhysicalDevice& device) {
		SwapChainSupportDetails details;
		details.capabilities = device.getSurfaceCapabilitiesKHR(surface);
		details.formats = device.getSurfaceFormatsKHR(surface);
		details.presentModes = device.getSurfacePresentModesKHR(surface);

		return details;
	}

	vk::SurfaceFormatKHR VulkanContext::chooseSwapChainSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
		for (const auto& availableFormat : availableFormats) {
			if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
				return availableFormat;
			}
		}

		return availableFormats[0];
	}

	vk::PresentModeKHR VulkanContext::chooseSwapChainPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
		for (const auto& availablePresentMode : availablePresentModes) {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox) {
				return availablePresentMode;
			}
		}

		return vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D VulkanContext::chooseSwapChainExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
		if (capabilities.currentExtent.width != UINT32_MAX) {
			return capabilities.currentExtent;
		}
		else {
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);

			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	bool VulkanContext::checkValidationLayerSupport() {
		auto availableLayers = vk::enumerateInstanceLayerProperties();

		printf("%u validation layers requested:\n", static_cast<uint32_t>(validationLayers.size()));
		for (const auto& layer : validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layer, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			printf(" [%c] %s\n", layerFound ? 'x' : ' ', layer);
			if (!layerFound) {
				return false;
			}
		}

		return true;
	}

	std::vector<const char*> VulkanContext::getRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (enableValidationLayers) {
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	vk::Format VulkanContext::findSupportedFormat(const std::vector<vk::Format>& candidates, vk::ImageTiling tiling, vk::FormatFeatureFlags features) {
		for (vk::Format format : candidates) {
			vk::FormatProperties properties = physicalDevice.getFormatProperties(format);
			if ((tiling == vk::ImageTiling::eLinear && (properties.linearTilingFeatures & features) == features)
				|| (tiling == vk::ImageTiling::eOptimal && (properties.optimalTilingFeatures & features) == features)) {
				return format;
			}
		}
		assert(false);
	}

	vk::Format VulkanContext::findDepthFormat() {
		return findSupportedFormat(
			{ vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint },
			vk::ImageTiling::eOptimal,
			vk::FormatFeatureFlagBits::eDepthStencilAttachment
		);
	}

	bool VulkanContext::hasStencilComponent(vk::Format format) {
		return format == vk::Format::eD32SfloatS8Uint || format == vk::Format::eD24UnormS8Uint;
	}

	void VulkanContext::createLogicalDevice() {
		queueFamilyIndices = findQueueFamilies(physicalDevice);
		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
		const std::vector<uint32_t> allQueueFamilies = {
			queueFamilyIndices.graphics.value(),
			queueFamilyIndices.present.value(),
			queueFamilyIndices.transfer.value()
		};
		std::vector<uint32_t> uniqueQueueFamilies;
		for (uint32_t queueFamily : allQueueFamilies) {
			if (!uniqueQueueFamilies.empty()) {
				bool found = false;
				// See if queue family index is already in the unique list
				for (uint32_t uniqueQueueFamily : uniqueQueueFamilies) {
					if (uniqueQueueFamily == queueFamily) {
						found = true;
						break;
					}
				}
				if (found) {
					continue;
				}
			}
			uniqueQueueFamilies.push_back(queueFamily);
		}
		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			vk::DeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.setQueueFamilyIndex(queueFamily);
			queueCreateInfo.setQueueCount(1);
			queueCreateInfo.setQueuePriorities(queuePriority);
			queueCreateInfos.push_back(queueCreateInfo);
		}

		vk::PhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;
		vk::DeviceCreateInfo createInfo{};
		createInfo.setQueueCreateInfos(queueCreateInfos);
		createInfo.setPEnabledFeatures(&deviceFeatures);
		createInfo.setPEnabledExtensionNames(deviceExtensions);

		device = physicalDevice.createDevice(createInfo);
		assert(device);
		graphicsQueue = device.getQueue(queueFamilyIndices.graphics.value(), 0);
		assert(graphicsQueue);
		presentQueue = device.getQueue(queueFamilyIndices.present.value(), 0);
		assert(presentQueue);
		transferQueue = device.getQueue(queueFamilyIndices.transfer.value(), 0);
		assert(presentQueue);
		printf("Created queues\n");
		printf(" Graphics queue: %d\n", queueFamilyIndices.graphics.value());
		printf(" Present queue: %d\n", queueFamilyIndices.present.value());
		printf(" Transfer queue: %d\n", queueFamilyIndices.transfer.value());
	}

	void VulkanContext::createVmaAllocator() {
		VmaAllocatorCreateInfo allocatorInfo = {};
		allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorInfo.physicalDevice = physicalDevice;
		allocatorInfo.device = device;
		allocatorInfo.instance = instance;
		
		vmaCreateAllocator(&allocatorInfo, &allocator);
	}

	void VulkanContext::createSwapChain() {
		printf("Creating swap chain\n");
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

		vk::SurfaceFormatKHR surfaceFormat = chooseSwapChainSurfaceFormat(swapChainSupport.formats);
		printf("Selected format: %u with color space: %u\n", static_cast<uint32_t>(surfaceFormat.format), static_cast<uint32_t>(surfaceFormat.colorSpace));
		vk::PresentModeKHR presentMode = chooseSwapChainPresentMode(swapChainSupport.presentModes);
		printf("Selected present mode: %u\n", static_cast<uint32_t>(presentMode));
		vk::Extent2D extent = chooseSwapChainExtent(swapChainSupport.capabilities);
		printf("Selected extent: %dx%d\n", extent.width, extent.height);

		uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
		if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
			imageCount = swapChainSupport.capabilities.maxImageCount;
		}

		printf("Using %d images\n", imageCount);

		vk::SwapchainCreateInfoKHR createInfo{};
		createInfo.surface = surface;
		createInfo.minImageCount = imageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;
		createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;

		uint32_t indices[] = { queueFamilyIndices.graphics.value(), queueFamilyIndices.present.value() };
		if (queueFamilyIndices.graphics.value() != queueFamilyIndices.present.value()) {
			createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			createInfo.setPQueueFamilyIndices(indices);
		}
		else {
			createInfo.imageSharingMode = vk::SharingMode::eExclusive;
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
		createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque;
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;
		createInfo.oldSwapchain = nullptr;

		swapChain = device.createSwapchainKHR(createInfo);
		assert(swapChain);

		swapChainImages = device.getSwapchainImagesKHR(swapChain);
		swapChainFormat = surfaceFormat.format;
		swapChainExtent = extent;
	}

	void VulkanContext::createSwapChainImageViews() {
		swapChainImageViews.resize(swapChainImages.size());
		for (uint32_t i = 0; i < swapChainImages.size(); i++) {
			swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainFormat, vk::ImageAspectFlagBits::eColor);
		}
	}

	void VulkanContext::createRenderPass() {
		vk::AttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainFormat;
		colorAttachment.samples = vk::SampleCountFlagBits::e1;
		colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
		colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
		colorAttachment.finalLayout = vk::ImageLayout::ePresentSrcKHR;

		vk::AttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = vk::ImageLayout::eColorAttachmentOptimal;

		vk::AttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = vk::SampleCountFlagBits::e1;
		depthAttachment.loadOp = vk::AttachmentLoadOp::eClear;
		depthAttachment.storeOp = vk::AttachmentStoreOp::eDontCare;
		depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
		depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
		depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
		depthAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		vk::AttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

		std::array<vk::AttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

		vk::SubpassDescription subpass{};
		subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
		subpass.setColorAttachments(colorAttachmentRef);
		subpass.pDepthStencilAttachment = &depthAttachmentRef;

		vk::SubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.srcAccessMask = vk::AccessFlagBits(0);
		dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
		dependency.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		vk::RenderPassCreateInfo renderPassInfo{};
		renderPassInfo.setAttachments(attachments);
		renderPassInfo.setSubpasses(subpass);
		renderPassInfo.setDependencies(dependency);

		renderPass = device.createRenderPass(renderPassInfo);
		assert(renderPass);
	}

	void VulkanContext::createDescriptorSetLayout() {
		vk::DescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		vk::DescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.stageFlags = vk::ShaderStageFlagBits::eFragment;
		samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional

		std::array<vk::DescriptorSetLayoutBinding, 2> layoutBindings = {
			uboLayoutBinding,
			samplerLayoutBinding
		};

		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		layoutInfo.pBindings = layoutBindings.data();

		descriptorSetLayout = device.createDescriptorSetLayout(layoutInfo);
		assert(descriptorSetLayout);

	}

	void VulkanContext::createGraphicsPipeline() {
		const std::string vertShaderFile = "assets/shaders/compiled/simple.vert.spv";
		auto vertShaderCode = readFile(vertShaderFile);

		const std::string fragShaderFile = "assets/shaders/compiled/simple.frag.spv";
		auto fragShaderCode = readFile(fragShaderFile);

		vk::ShaderModule vertShaderModule = createShaderModule(vertShaderCode);
		vk::ShaderModule fragShaderModule = createShaderModule(fragShaderCode);

		vk::PipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
		vertShaderStageInfo.module = vertShaderModule;
		vertShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		std::array<vk::VertexInputBindingDescription, 1> bindingDescriptions = Vertex::getBindingDescriptions();
		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions = Vertex::getAttributeDescriptions();

		vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescriptions.size());;
		vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		vk::PipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		vk::PipelineViewportStateCreateInfo viewportState{};
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr; // Dynamic
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr; // Dynamic

		vk::PipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = vk::PolygonMode::eFill;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = vk::CullModeFlagBits::eBack;
		rasterizer.frontFace = vk::FrontFace::eCounterClockwise;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		vk::PipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = vk::SampleCountFlagBits::e1;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		vk::PipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = vk::CompareOp::eLess;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.minDepthBounds = 0.0f; // Optional
		depthStencil.maxDepthBounds = 1.0f; // Optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = vk::StencilOpState{}; // Optional
		depthStencil.back = vk::StencilOpState{}; // Optional

		vk::PipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = vk::BlendFactor::eOne; // Optional
		colorBlendAttachment.dstColorBlendFactor = vk::BlendFactor::eZero; // Optional
		colorBlendAttachment.colorBlendOp = vk::BlendOp::eAdd; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = vk::BlendFactor::eOne; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = vk::BlendFactor::eZero; // Optional
		colorBlendAttachment.alphaBlendOp = vk::BlendOp::eAdd; // Optional

		vk::PipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = vk::LogicOp::eCopy; // Optional
		colorBlending.setAttachments(colorBlendAttachment);
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		vk::DynamicState dynamicStates[] = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};

		vk::PipelineDynamicStateCreateInfo dynamicStateInfo{};
		dynamicStateInfo.dynamicStateCount = sizeof(dynamicStates) / sizeof(dynamicStates[0]);
		dynamicStateInfo.pDynamicStates = dynamicStates;

		vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.setSetLayouts(descriptorSetLayout);
		pipelineLayoutInfo.setPushConstantRanges({});

		pipelineLayout = device.createPipelineLayout(pipelineLayoutInfo);
		assert(pipelineLayout);

		vk::GraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.stageCount = sizeof(shaderStages) / sizeof(shaderStages[0]);
		pipelineInfo.pStages = shaderStages;
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicStateInfo;
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = nullptr; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		vk::ResultValue<vk::Pipeline> result = device.createGraphicsPipeline(nullptr, pipelineInfo);
		assert(result.result == vk::Result::eSuccess);
		graphicsPipeline = result.value;

		device.destroyShaderModule(vertShaderModule);
		device.destroyShaderModule(fragShaderModule);
	}

	void VulkanContext::createFramebuffers() {
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++) {
			std::array<vk::ImageView, 2> attachments = {
				swapChainImageViews[i],
				depthImageView
			};

			vk::FramebufferCreateInfo framebufferInfo{};
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			swapChainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
			assert(swapChainFramebuffers[i]);
		}
	}

	void VulkanContext::createCommandPools() {
		{
			// Graphics command pool
			vk::CommandPoolCreateInfo poolInfo{};
			poolInfo.queueFamilyIndex = queueFamilyIndices.graphics.value();
			poolInfo.flags = vk::CommandPoolCreateFlagBits(0); // optional

			graphicsCommandPool = device.createCommandPool(poolInfo);
			assert(graphicsCommandPool);
		}
		{
			// Transfer command pool
			vk::CommandPoolCreateInfo poolInfo{};
			poolInfo.queueFamilyIndex = queueFamilyIndices.transfer.value();
			poolInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;

			transferCommandPool = device.createCommandPool(poolInfo);
			assert(transferCommandPool);
		}
	}

	void VulkanContext::createDepthResources() {
		vk::Format depthFormat = findDepthFormat();
		VK_CHECK(createImage2D(
			swapChainExtent.width, swapChainExtent.height,
			depthFormat,
			vk::ImageTiling::eOptimal,
			vk::ImageUsageFlagBits::eDepthStencilAttachment,
			VMA_MEMORY_USAGE_GPU_ONLY,
			depthImage, depthImageAlloc
		));
		depthImageView = createImageView(depthImage, depthFormat, vk::ImageAspectFlagBits::eDepth);

		transitionImageLayout(depthImage, depthFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthStencilAttachmentOptimal);

	}

	void VulkanContext::createSampledImage() {
		int texWidth;
		int texHeight;
		int texChannels;
		stbi_uc* pixels = stbi_load("assets/textures/texture.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		assert(pixels);

		vk::DeviceSize imageSize = static_cast<uint64_t>(texWidth) * static_cast<uint64_t>(texHeight) * 4;

		vk::Buffer stagingBuffer;
		VmaAllocation stagingBufferAlloc;
		VK_CHECK(createBuffer(
			imageSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			queueFamilyIndices.transfer.value(),
			VMA_MEMORY_USAGE_CPU_ONLY,
			stagingBuffer, stagingBufferAlloc
		));

		void* mappedData;
		vmaMapMemory(allocator, stagingBufferAlloc, &mappedData);
		memcpy(mappedData, pixels, static_cast<size_t>(imageSize));
		vmaUnmapMemory(allocator, stagingBufferAlloc);
		stbi_image_free(pixels);

		sampledImage = createTexture((uint32_t)texWidth, (uint32_t)texHeight, 1, Texture::Format::R8G8B8A8Srgb, Texture::FlagBits::Sampled | Texture::FlagBits::TransferDst, Texture::SampleCount::Samples1, Texture::MemoryUsage::GpuOnly);

		vk::CommandBuffer commandBuffer = beginOneTimeCommandBuffer(graphicsCommandPool);
		transitionImageLayout(
			sampledImage->vk.image, vk::Format::eR8G8B8A8Srgb,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal,
			commandBuffer
		);
		copyBufferToImage(
			stagingBuffer, sampledImage->vk.image,
			static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight),
			commandBuffer
		);
		transitionImageLayout(
			sampledImage->vk.image, vk::Format::eR8G8B8A8Srgb,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal,
			commandBuffer
		);
		endOneTimeCommandBuffer(commandBuffer, graphicsCommandPool, graphicsQueue);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAlloc);
	}

	void VulkanContext::createTextureSampler() {
		vk::PhysicalDeviceProperties properties = physicalDevice.getProperties();
		vk::PhysicalDeviceFeatures features = physicalDevice.getFeatures();

		vk::SamplerCreateInfo samplerInfo{};
		samplerInfo.magFilter = vk::Filter::eLinear;
		samplerInfo.minFilter = vk::Filter::eLinear;
		samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
		samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
		if (features.samplerAnisotropy) {
			samplerInfo.anisotropyEnable = VK_TRUE;
			samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
		}
		else {
			samplerInfo.anisotropyEnable = VK_FALSE;
			samplerInfo.maxAnisotropy = 1.0f;
		}
		samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = vk::CompareOp::eAlways;
		samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		textureSampler = device.createSampler(samplerInfo);
	}


	void VulkanContext::createUniformBuffers() {
		vk::DeviceSize bufferSize = sizeof(UniformBufferObject);

		uniformBuffers.resize(swapChainImages.size());
		uniformBufferAllocs.resize(uniformBuffers.size());

		for (size_t i = 0; i < uniformBuffers.size(); i++) {
			VK_CHECK(createBuffer(
				bufferSize,
				vk::BufferUsageFlagBits::eUniformBuffer,
				queueFamilyIndices.graphics.value(),
				VMA_MEMORY_USAGE_CPU_TO_GPU,
				uniformBuffers[i], uniformBufferAllocs[i]
			));
		}
	}

	void VulkanContext::createDescriptorPool() {
		std::array<vk::DescriptorPoolSize, 2> poolSizes = {};

		poolSizes[0].type = vk::DescriptorType::eUniformBuffer;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

		poolSizes[1].type = vk::DescriptorType::eCombinedImageSampler;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

		vk::DescriptorPoolCreateInfo poolInfo{};
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

		descriptorPool = device.createDescriptorPool(poolInfo);
		assert(descriptorPool);
	}

	void VulkanContext::createDescriptorSets() {
		std::vector<vk::DescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
		vk::DescriptorSetAllocateInfo allocInfo{};
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = (uint32_t)swapChainImages.size();
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(swapChainImages.size());
		descriptorSets = device.allocateDescriptorSets(allocInfo);

		for (size_t i = 0; i < swapChainImages.size(); i++) {

			std::array<vk::WriteDescriptorSet, 2> descriptorWrites{};

			vk::DescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = vk::DescriptorType::eUniformBuffer;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;
			descriptorWrites[0].pImageInfo = nullptr; // Optional
			descriptorWrites[0].pTexelBufferView = nullptr; // Optional

			vk::DescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
			imageInfo.imageView = sampledImage->vk.imageView;
			imageInfo.sampler = textureSampler;

			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = vk::DescriptorType::eCombinedImageSampler;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pBufferInfo = nullptr; // Optional
			descriptorWrites[1].pImageInfo = &imageInfo;
			descriptorWrites[1].pTexelBufferView = nullptr; // Optional

			device.updateDescriptorSets(descriptorWrites, {});
		}
	}

	void VulkanContext::createCommandBuffers() {
		commandBuffers.resize(swapChainFramebuffers.size());

		vk::Viewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 1.0f;
		viewport.maxDepth = 0.0f;

		vk::Rect2D scissor{};
		scissor.offset = vk::Offset2D{ 0, 0 };
		scissor.extent = swapChainExtent;

		vk::CommandBufferAllocateInfo allocInfo{};
		allocInfo.commandPool = graphicsCommandPool;
		allocInfo.level = vk::CommandBufferLevel::ePrimary;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		commandBuffers = device.allocateCommandBuffers(allocInfo);

		for (size_t i = 0; i < commandBuffers.size(); i++) {
			vk::CommandBufferBeginInfo beginInfo{};
			beginInfo.flags = vk::CommandBufferUsageFlagBits(0); // optional
			beginInfo.pInheritanceInfo = nullptr; // optional

			commandBuffers[i].begin(beginInfo);

			std::array<float, 4> clearColorValues = { 0.0f, 0.0f, 0.0f, 1.0f };
			std::array<vk::ClearValue, 2> clearValues = {
				vk::ClearColorValue(clearColorValues),
				vk::ClearDepthStencilValue(1.0f, 0)
			};

			vk::RenderPassBeginInfo renderPassInfo{};
			renderPassInfo.renderPass = renderPass;
			renderPassInfo.framebuffer = swapChainFramebuffers[i];
			renderPassInfo.renderArea.offset = vk::Offset2D{ 0, 0 };
			renderPassInfo.renderArea.extent = swapChainExtent;
			renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
			renderPassInfo.pClearValues = clearValues.data();

			commandBuffers[i].beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);
			commandBuffers[i].setViewport(0, viewport);
			commandBuffers[i].setScissor(0, scissor);
			commandBuffers[i].bindPipeline(vk::PipelineBindPoint::eGraphics, graphicsPipeline);
			std::vector<vk::Buffer> vertexBuffers = { vb->vk.buffer };
			std::vector<vk::DeviceSize> offsets = { 0 };
			commandBuffers[i].bindVertexBuffers(0, vertexBuffers, offsets);
			commandBuffers[i].bindIndexBuffer(ib->vk.buffer, 0, vk::IndexType::eUint16);
			commandBuffers[i].bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, descriptorSets[i], {});
			commandBuffers[i].drawIndexed(static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
			commandBuffers[i].endRenderPass();

			commandBuffers[i].end();
		}
	}

	void VulkanContext::createSyncObjects() {
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
		imagesInFlight.resize(swapChainImages.size(), nullptr);

		vk::SemaphoreCreateInfo semaphoreInfo{};
		vk::FenceCreateInfo fenceInfo{};
		fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
			assert(imageAvailableSemaphores[i]);
			renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
			assert(renderFinishedSemaphores[i]);
			inFlightFences[i] = device.createFence(fenceInfo);
			assert(inFlightFences[i]);
		}

		vk::FenceCreateInfo bufferCopyFenceInfo{};
		bufferCopyFence = device.createFence(bufferCopyFenceInfo);
		assert(bufferCopyFence);
	}

	void VulkanContext::cleanupSwapChain() {
		device.destroyImageView(depthImageView);
		vmaDestroyImage(allocator, depthImage, depthImageAlloc);

		for (auto& framebuffer : swapChainFramebuffers) {
			device.destroyFramebuffer(framebuffer);
		}

		device.freeCommandBuffers(graphicsCommandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
		device.destroyRenderPass(renderPass);

		for (auto& imageView : swapChainImageViews) {
			device.destroyImageView(imageView);
		}

		device.destroySwapchainKHR(swapChain);

		for (size_t i = 0; i < uniformBuffers.size(); i++) {
			vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBufferAllocs[i]);
		}
		device.destroyDescriptorPool(descriptorPool);
	}

	void VulkanContext::recreateSwapChain() {
		int currentWidth = 0, currentHeight = 0;
		glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
		while (currentWidth == 0 || currentHeight == 0) {
			glfwGetFramebufferSize(window, &currentWidth, &currentHeight);
			glfwWaitEvents();
		}

		device.waitIdle();

		cleanupSwapChain();

		createSwapChain();
		createSwapChainImageViews();
		createRenderPass();
		createDepthResources();
		createFramebuffers();
		createUniformBuffers();
		createDescriptorPool();
		createDescriptorSets();
		createCommandBuffers();
	}

	void VulkanContext::drawFrame() {
		uint32_t frameIndex = currentFrame % MAX_FRAMES_IN_FLIGHT;
		VK_CHECK(device.waitForFences(inFlightFences[frameIndex], VK_TRUE, UINT64_MAX))

		uint32_t imageIndex;
		// Have to use the plain C functions here, 
		// because the Vulkan HPP functions will throw an exception
		// on VK_ERROR_OUT_OF_DATE_KHR
		vk::Result acquireResult = static_cast<vk::Result>(vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[frameIndex], nullptr, &imageIndex));
		if (acquireResult == vk::Result::eErrorOutOfDateKHR) {
			recreateSwapChain();
			return;
		}
		assert(acquireResult == vk::Result::eSuccess || acquireResult == vk::Result::eSuboptimalKHR);

		// Check if a previous frame is using this image (i.e. there is its fence to wait on)
		if (imagesInFlight[imageIndex]) {
			VK_CHECK(device.waitForFences(imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX))
		}
		// Mark the image as now being in use by this frame
		imagesInFlight[imageIndex] = inFlightFences[frameIndex];

		vk::Semaphore waitSemaphores[] = { imageAvailableSemaphores[frameIndex] };
		vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };
		vk::Semaphore signalSemaphores[] = { renderFinishedSemaphores[frameIndex] };

		updateUniformBuffer(imageIndex);

		vk::SubmitInfo submitInfo{};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[imageIndex];
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		device.resetFences(inFlightFences[frameIndex]);

		graphicsQueue.submit(submitInfo, inFlightFences[frameIndex]);

		vk::SwapchainKHR swapChains[] = { swapChain };
		vk::PresentInfoKHR presentInfo{};
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // optional

		// Have to use the plain C functions here, 
		// because the Vulkan HPP functions will throw an exception
		// on VK_ERROR_OUT_OF_DATE_KHR
		vk::Result presentResult = static_cast<vk::Result>(vkQueuePresentKHR(presentQueue, (VkPresentInfoKHR*)(&presentInfo)));
		if (presentResult == vk::Result::eErrorOutOfDateKHR || presentResult == vk::Result::eSuboptimalKHR) {
			recreateSwapChain();
		}
		else {
			assert(presentResult == vk::Result::eSuccess);
		}

		currentFrame++;
	}

	void VulkanContext::updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float elapsedTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4(1.0f), elapsedTime * glm::radians(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		ubo.view = glm::lookAt(glm::vec3(sinf(elapsedTime * 0.5f) * 1.5f, sinf(elapsedTime * 0.3f), -2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		constexpr float fovy = glm::radians(45.0f);
		const float aspect = (float)swapChainExtent.width / (float)swapChainExtent.height;
		const float f = 1.0f / tanf(fovy * 0.5f);
		const float nearPlane = 0.1f;
		ubo.proj = {
			{f / aspect, 0.0f, 0.0f, 0.0f}, // first COLUMN
			{ 0.0f, f, 0.0f, 0.0f }, // second COLUMN
			{ 0.0f, 0.0f, 0.0f, -1.0f }, // third COLUMN
			{ 0.0f, 0.0f, nearPlane, 0.0f } // fourth COLUMN
		};

		void* mappedData;
		vmaMapMemory(
			allocator,
			uniformBufferAllocs[currentImage],
			&mappedData
		);
		memcpy(mappedData, &ubo, sizeof(ubo));
		vmaUnmapMemory(allocator, uniformBufferAllocs[currentImage]);
	}

	void VulkanContext::update() {
		drawFrame();
		device.waitIdle();
	}

	VertexBuffer* VulkanContext::createVertexBuffer(const Vertex* vertices, uint16_t vertexCount) {
		const vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;

		std::vector<uint32_t> stagingQueueIndices = { queueFamilyIndices.transfer.value() };
		vk::Buffer stagingBuffer;
		VmaAllocation stagingBufferAlloc;
		VK_CHECK(createBuffer(
			bufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			stagingQueueIndices,
			VMA_MEMORY_USAGE_CPU_ONLY,
			stagingBuffer, stagingBufferAlloc
		));

		void* mappedData;
		vmaMapMemory(allocator, stagingBufferAlloc, &mappedData);
		memcpy(mappedData, vertices, bufferSize);
		vmaUnmapMemory(allocator, stagingBufferAlloc);

		VertexBuffer* vertexBuffer = new VertexBuffer();

		std::vector<uint32_t> queueIndices = { queueFamilyIndices.graphics.value(), queueFamilyIndices.transfer.value() };
		VK_CHECK(createBuffer(
			bufferSize,
			vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			queueIndices,
			VMA_MEMORY_USAGE_GPU_ONLY,
			vertexBuffer->vk.buffer, vertexBuffer->vk.bufferAlloc
		));

		copyBuffer(stagingBuffer, vertexBuffer->vk.buffer, bufferSize);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAlloc);

		return vertexBuffer;
	}

	inline void VulkanContext::destroyVertexBuffer(VertexBuffer* vertexBuffer) {
		vmaDestroyBuffer(allocator, vertexBuffer->vk.buffer, vertexBuffer->vk.bufferAlloc);
		delete vertexBuffer;
	}

	IndexBuffer* VulkanContext::createIndexBuffer(const uint16_t* indices, uint32_t indexCount) {
		const vk::DeviceSize bufferSize = sizeof(indices[0]) * indexCount;

		std::vector<uint32_t> stagingQueueIndices = { queueFamilyIndices.transfer.value() };
		vk::Buffer stagingBuffer;
		VmaAllocation stagingBufferAlloc;
		VK_CHECK(createBuffer(
			bufferSize,
			vk::BufferUsageFlagBits::eTransferSrc,
			stagingQueueIndices,
			VMA_MEMORY_USAGE_CPU_ONLY,
			stagingBuffer, stagingBufferAlloc
		));


		void* mappedData;
		vmaMapMemory(allocator, stagingBufferAlloc, &mappedData);
		memcpy(mappedData, indices, bufferSize);
		vmaUnmapMemory(allocator, stagingBufferAlloc);

		IndexBuffer* indexBuffer = new IndexBuffer();

		std::vector<uint32_t> queueIndices = { queueFamilyIndices.graphics.value(), queueFamilyIndices.transfer.value() };
		VK_CHECK(createBuffer(
			bufferSize,
			vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst,
			queueIndices,
			VMA_MEMORY_USAGE_GPU_ONLY,
			indexBuffer->vk.buffer, indexBuffer->vk.bufferAlloc
		));

		copyBuffer(stagingBuffer, indexBuffer->vk.buffer, bufferSize);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAlloc);

		return indexBuffer;
	}

	inline void VulkanContext::destroyIndexBuffer(IndexBuffer* indexBuffer) {
		vmaDestroyBuffer(allocator, indexBuffer->vk.buffer, indexBuffer->vk.bufferAlloc);
		delete indexBuffer;
	}

	Texture* VulkanContext::createTexture(uint32_t width, uint32_t height, uint32_t depth, Texture::Format format, Texture::Flags flags, Texture::SampleCount sampleCount, Texture::MemoryUsage memoryUsage) {
		vk::Format vkFormat = convertToVkFormat(format);
		vk::ImageUsageFlags usage;
		assert(!(flags.RenderTarget && (flags.Depth || flags.Stencil)));
		usage |= flags.RenderTarget ? vk::ImageUsageFlagBits::eColorAttachment : vk::ImageUsageFlagBits(0);
		usage |= (flags.Depth || flags.Stencil) ? vk::ImageUsageFlagBits::eDepthStencilAttachment : vk::ImageUsageFlagBits(0);
		usage |= flags.Sampled ? vk::ImageUsageFlagBits::eSampled : vk::ImageUsageFlagBits(0);
		usage |= flags.TransferSrc ? vk::ImageUsageFlagBits::eTransferSrc : vk::ImageUsageFlagBits(0);
		usage |= flags.TransferDst ? vk::ImageUsageFlagBits::eTransferDst : vk::ImageUsageFlagBits(0);

		assert(sampleCount == Texture::SampleCount::Samples1); // TODO

		vk::ImageCreateInfo imageInfo{};
		imageInfo.imageType = vk::ImageType::e2D;
		imageInfo.extent.width = width;
		imageInfo.extent.height = height;
		imageInfo.extent.depth = depth;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = vkFormat;
		imageInfo.tiling = vk::ImageTiling::eOptimal; // TODO
		imageInfo.usage = usage;
		imageInfo.sharingMode = vk::SharingMode::eExclusive;
		imageInfo.samples = vk::SampleCountFlagBits::e1;
		imageInfo.flags = vk::ImageCreateFlags(); // Optional

		VmaMemoryUsage vmaMemoryUsage = convertToVmaMemoryUsage(memoryUsage);
		VmaAllocationCreateInfo allocInfo = {};
		allocInfo.usage = vmaMemoryUsage;

		Texture* texture = new Texture();
		texture->format = format;
		texture->flags = flags;
		texture->sampleCount = sampleCount;
		texture->memoryUsage = memoryUsage;
		VkResult result = vmaCreateImage(allocator, (VkImageCreateInfo*)&imageInfo, &allocInfo, (VkImage*)&(texture->vk.image), &(texture->vk.imageAlloc), nullptr);
		assert(vk::Result(result) == vk::Result::eSuccess);
		vk::ImageAspectFlags aspectFlags;
		aspectFlags |= flags.RenderTarget ? vk::ImageAspectFlagBits::eColor : vk::ImageAspectFlagBits(0);
		aspectFlags |= flags.Sampled && !(flags.Depth || flags.Stencil) ? vk::ImageAspectFlagBits::eColor : vk::ImageAspectFlagBits(0);
		aspectFlags |= flags.Depth ? vk::ImageAspectFlagBits::eDepth : vk::ImageAspectFlagBits(0);
		aspectFlags |= flags.Stencil ? vk::ImageAspectFlagBits::eStencil : vk::ImageAspectFlagBits(0);
		texture->vk.imageView = createImageView(texture->vk.image, vkFormat, aspectFlags);

		return texture;
	}

	void VulkanContext::destroyTexture(Texture* texture) {
		device.destroyImageView(texture->vk.imageView);
		vmaDestroyImage(allocator, texture->vk.image, texture->vk.imageAlloc);
		delete texture;
	}

	void VulkanContext::cleanup() {
		// vulkan
		cleanupSwapChain();

		device.destroyPipeline(graphicsPipeline);
		device.destroyPipelineLayout(pipelineLayout);
		device.destroyDescriptorSetLayout(descriptorSetLayout);

		destroyVertexBuffer(vb);
		destroyIndexBuffer(ib);
		destroyTexture(sampledImage);
		device.destroySampler(textureSampler);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			device.destroySemaphore(imageAvailableSemaphores[i]);
			device.destroySemaphore(renderFinishedSemaphores[i]);
			device.destroyFence(inFlightFences[i]);
		}
		device.destroyFence(bufferCopyFence);

		device.destroyCommandPool(graphicsCommandPool);
		device.destroyCommandPool(transferCommandPool);
		vmaDestroyAllocator(allocator);
		device.destroy();
		if (enableValidationLayers) {
			DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
		}
		instance.destroySurfaceKHR(surface);
		instance.destroy();

		// glfw
		glfwDestroyWindow(window);

		glfwTerminate();
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanContext::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

		printf("validation layer: %s\n", pCallbackData->pMessage);

		if (messageSeverity == VkDebugUtilsMessageSeverityFlagBitsEXT::VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
			//assert(false);
		}

		return VK_FALSE;
	}

	void VulkanContext::resizeFramebuffer(uint16_t width, uint16_t height) noexcept {
		recreateSwapChain();
	}

	vk::Format VulkanContext::convertToVkFormat(Texture::Format format) {
		switch (format) {
			case Texture::Format::R8G8B8A8Srgb: return vk::Format::eR8G8B8A8Srgb;
			case Texture::Format::D32Sfloat: return vk::Format::eD32Sfloat;
			default: 
				assert(false && "Not implemented (yet)"); 
				return vk::Format::eUndefined;
		}
	}
	VmaMemoryUsage VulkanContext::convertToVmaMemoryUsage(Texture::MemoryUsage memoryUsage) {
		switch (memoryUsage) {
			case Texture::MemoryUsage::CpuToGpu: return VMA_MEMORY_USAGE_CPU_TO_GPU;
			case Texture::MemoryUsage::GpuOnly: return VMA_MEMORY_USAGE_GPU_ONLY;
			default:
				assert(false && "Not implemented (yet)");
				return VMA_MEMORY_USAGE_UNKNOWN;
		}
	}
}
