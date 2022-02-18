#include "Mesh.hpp"

#include "GfxContext.hpp"

namespace vesuvio {

	Mesh::Mesh(GfxContext* gfxContext) 
	: gfxContext(gfxContext)
	, storage(StorageInfo::Synced)
	{

	}

	Mesh::~Mesh() {

	}

	void Mesh::setData(const glm::vec3* posData, const Remainder* remainderData, uint16_t vertexCount, const uint16_t* indicesData, uint32_t indexCount, StorageInfo storageInfo) {
		storage = storageInfo;

		pos.clear();
		remainder.clear();
		indices.clear();
		if (storageInfo == StorageInfo::Synced) {
			pos.reserve(vertexCount);
			for (uint16_t i = 0; i < vertexCount; i++) {
				pos.push_back(posData[i]);
			}

			remainder.reserve(vertexCount);
			for (uint16_t i = 0; i < vertexCount; i++) {
				remainder.push_back(remainderData[i]);
			}

			indices.reserve(indexCount);
			for (uint32_t i = 0; i < indexCount; i++) {
				indices.push_back(indicesData[i]);
			}
		}

		switch(gfxContext->getGfxBackend())
		{
#if VSV_GFX_BACKEND(VULKAN)
			case GfxContext::GfxBackend::Vulkan:
				vkImpl.setData(posData, remainderData, vertexCount, indicesData, indexCount, storageInfo);
				break;
#endif
			default:
				return;
		}
	}

	void Mesh::VulkanImpl::setData(const glm::vec3* posData, const Remainder* remainderData, uint16_t vertexCount, const uint16_t* indicesData, uint32_t indexCount, StorageInfo storageInfo) {

	}

	inline std::array<vk::VertexInputBindingDescription, 2> Mesh::getBindingDescriptions() {
		std::array<vk::VertexInputBindingDescription, 2> bindingDescriptions{};
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(pos);
		bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;


		vk::VertexInputBindingDescription bindingDescriptionRemainder{};
		bindingDescriptions[1].binding = 1;
		bindingDescriptions[1].stride = sizeof(Remainder);
		bindingDescriptions[1].inputRate = vk::VertexInputRate::eVertex;

		return bindingDescriptions;
	}

	inline std::array<vk::VertexInputAttributeDescription, 3> Mesh::getAttributeDescriptions() {
		std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions{};

		attributeDescriptions[0].binding = 1;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[0].offset = offsetof(Vertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
		attributeDescriptions[1].offset = offsetof(Vertex, color);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
		attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

		return attributeDescriptions;
	}
}