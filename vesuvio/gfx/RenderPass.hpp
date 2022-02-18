#pragma once

#include "GfxContext.hpp"

#if VSV_GFX_BACKEND(VULKAN)
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#endif

#include "Texture.hpp"

namespace vesuvio {
	struct RenderPass
	{
		std::vector<Texture*> colorAttachments;
		Texture* depthAttachment;
#if VSV_GFX_BACKEND(VULKAN)
		struct
		{
			vk::RenderPass renderPass;
		} vk;
#endif
	};

}