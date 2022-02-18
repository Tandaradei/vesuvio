#pragma once

#include <stdint.h>

#define VSV_GFX_BACKEND(X) VSV_GFX_BACKEND_##X()


#define VSV_GFX_BACKEND_None() 1

#if defined(VSV_ENABLE_VULKAN)
#define VSV_GFX_BACKEND_VULKAN() 1
#else
#define VSV_GFX_BACKEND_VULKAN() 0
#endif

#if defined(VSV_ENABLE_D3D12)
#define VSV_GFX_BACKEND_D3D12() 1
#else
#define VSV_GFX_BACKEND_D3D12() 0
#endif

struct GLFWwindow;
struct Vertex;
#include "VertexBuffer.hpp"
#include "IndexBuffer.hpp"
#include "Texture.hpp"

namespace vesuvio {
	class GfxContext
	{
	public:
		enum class GfxBackend
		{
			None,
			Vulkan,
			D3D12,
		};
	public:
		virtual ~GfxContext() {}
		virtual void init(const char* appName, GLFWwindow* window) = 0;
		virtual void update() = 0;

		//virtual void createBuffer() = 0;
		virtual VertexBuffer* createVertexBuffer(const Vertex* vertices, uint16_t vertexCount) = 0;
		virtual void destroyVertexBuffer(VertexBuffer* vertexBuffer) = 0;
		virtual IndexBuffer* createIndexBuffer(const uint16_t* indices, uint32_t indexCount) = 0;
		virtual void destroyIndexBuffer(IndexBuffer* indexBuffer) = 0;

		virtual Texture* createTexture(uint32_t width, uint32_t height, uint32_t depth, Texture::Format format, Texture::Flags flags, Texture::SampleCount sampleCount, Texture::MemoryUsage memoryUsage) = 0;
		virtual void destroyTexture(Texture* texture) = 0;

		/*
		beginRenderPass(rp)
		setMaterial(m)
		setMeshBuffers(vb, ib)
		draw(indexCount)
		endRenderPass()
		*/

		virtual void resizeFramebuffer(uint16_t width, uint16_t height) = 0;
		virtual GfxBackend getGfxBackend() const = 0;
	};
}