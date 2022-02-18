#pragma once

#include "GfxContext.hpp"

#if VSV_GFX_BACKEND(VULKAN)
#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>
#endif


namespace vesuvio {

	struct Texture
	{
		enum class Format
		{
			R8G8B8A8Srgb,
			D32Sfloat
		};
		union Flags
		{
			Flags() : bits(0) {}
			Flags(const Flags& other) : bits(other.bits) {}
			Flags(uint32_t _bits) : bits(_bits) {}
			Flags& operator=(const Flags& other) { bits = other.bits; return *this; }
			struct
			{
				uint32_t Sampled : 1;
				uint32_t RenderTarget : 1;
				uint32_t Depth : 1;
				uint32_t Stencil : 1;
				uint32_t TransferSrc : 1;
				uint32_t TransferDst : 1;
			};
			uint32_t bits;
		};
		struct FlagBits
		{
			static constexpr uint32_t Sampled = 1 << 0;
			static constexpr uint32_t RenderTarget = 1 << 1;
			static constexpr uint32_t Depth = 1 << 2;
			static constexpr uint32_t Stencil = 1 << 3;
			static constexpr uint32_t TransferSrc = 1 << 4;
			static constexpr uint32_t TransferDst = 1 << 5;
		};
		enum class SampleCount
		{
			Samples1 = 1,
			Samples2 = 2,
			Samples4 = 4
		};
		// Could probably moved somewhere else as this is more generic than just for textures (buffers etc)
		enum class MemoryUsage 
		{
			GpuOnly,
			CpuToGpu
		};
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		Format format;
		Flags flags;
		SampleCount sampleCount;
		MemoryUsage memoryUsage;
#if VSV_GFX_BACKEND(VULKAN)
		struct
		{
			vk::Image image;
			VmaAllocation imageAlloc;
			vk::ImageView imageView;
			vk::ImageLayout layout;
		} vk;
#endif
	};

}