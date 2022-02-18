#pragma once

#include "GfxContext.hpp"

#if VSV_GFX_BACKEND(VULKAN)
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#endif

#include "Vertex.hpp"

namespace vesuvio {
	struct VertexBuffer
	{
		std::vector<Vertex> vertices;
#if VSV_GFX_BACKEND(VULKAN)
		struct 
		{
			vk::Buffer buffer;
			VmaAllocation bufferAlloc;
		} vk;
#endif
	};
}