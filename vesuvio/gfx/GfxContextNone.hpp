#pragma once

#include "GfxContext.hpp"

struct GLFWwindow;

namespace vesuvio {
	class GfxContextNone : public GfxContext
	{
	public:
		virtual ~GfxContextNone() {}
		virtual void init(const char* appName, GLFWwindow* window) override {}
		virtual void update() override {}

		VertexBuffer* createVertexBuffer(const Vertex* vertices, uint16_t vertexCount) override { return nullptr; };
		void destroyVertexBuffer(VertexBuffer* vertexBuffer) override {};
		IndexBuffer* createIndexBuffer(const uint16_t* indices, uint32_t indexCount) override { return nullptr; };
		void destroyIndexBuffer(IndexBuffer* vertexBuffer) override {};

		Texture* createTexture(uint32_t width, uint32_t height, uint32_t depth, Texture::Format format, Texture::Flags flags, Texture::SampleCount sampleCount, Texture::MemoryUsage memoryUsage) override { return nullptr; };
		void destroyTexture(Texture* texture) override {};

		virtual GfxBackend getGfxBackend() const override { return GfxContext::GfxBackend::None; }
		virtual void resizeFramebuffer(uint16_t width, uint16_t height) override {};
	};
}