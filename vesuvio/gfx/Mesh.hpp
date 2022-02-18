#pragma once

#include "GfxContext.hpp"

#if VSV_GFX_BACKEND(VULKAN)
#include <vulkan/vulkan.hpp>
#endif

#include "Vertex.hpp"

namespace vesuvio {
	class GfxContext;

	class Mesh
	{
		enum class StorageInfo
		{
			Synced, // mesh data will be present on CPU and GPU side and can be modified
			GpuOnly, // mesh data is only present on GPU and cannot be easily modified
		};

		std::vector<glm::vec3> pos;
		struct Remainder {
			glm::vec3 color;
			glm::vec2 texCoord;
		};
		std::vector<Remainder> remainder;
		std::vector<uint16_t> indices;

		Mesh(GfxContext* gfxContext);
		~Mesh();

		static std::array<vk::VertexInputBindingDescription, 2> getBindingDescriptions();
		static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions();
		// void create(const glm::vec3* posData, uint16_t posCount);
		void setData(const glm::vec3* posData, const Remainder* remainderData, uint16_t vertexCount, const uint16_t* indicesData, uint32_t indexCount, StorageInfo storageInfo);

	private:

		struct VulkanImpl
		{
			vk::Buffer posBuffer;
			vk::Buffer remainderBuffer;
			void setData(const glm::vec3* posData, const Remainder* remainderData, uint16_t vertexCount, const uint16_t* indicesData, uint32_t indexCount, StorageInfo storageInfo);
		};
#if VSV_GFX_BACKEND(VULKAN)
		VulkanImpl vkImpl;
#endif

	private:
		GfxContext* gfxContext;
		StorageInfo storage;
	};
}