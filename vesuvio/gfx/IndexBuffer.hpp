#pragma once

#include "GfxContext.hpp"

#if VSV_GFX_BACKEND(VULKAN)
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#endif

#include "Vertex.hpp"

namespace vesuvio {
	struct IndexBuffer
	{
		std::vector<uint16_t> indices;
#if VSV_GFX_BACKEND(VULKAN)
		struct
		{
			vk::Buffer buffer;
			VmaAllocation bufferAlloc;
		} vk;
#endif
	};
}