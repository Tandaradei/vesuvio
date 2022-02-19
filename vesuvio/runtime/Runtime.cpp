#include "Runtime.hpp"

#include <cassert>

#if VSV_GFX_BACKEND(VULKAN)
#include "VulkanContext.hpp"
#endif

#include "GfxContextNone.hpp"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace vesuvio {

	Runtime::Runtime()
		: updateFunc(nullptr)
		, window(nullptr)
		, gfx(nullptr)
	{
			
	}

	Runtime::~Runtime() {
		gfxTest.cleanup();
		if (gfx) {
			delete gfx;
		}
	}

	void Runtime::init(WindowInit windowInit, GfxInit gfxInit, UpdateFunc updateFn) {
		updateFunc = updateFn;

		glfwInit();

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		window = glfwCreateWindow((int)windowInit.width, (int)windowInit.height, windowInit.name, nullptr, nullptr);
		assert(window);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

		if (gfxInit.gfxBackend == GfxContext::GfxBackend::None) {
			gfx = new GfxContextNone();
		}
		else if (gfxInit.gfxBackend == GfxContext::GfxBackend::Vulkan) {
			#if VSV_GFX_BACKEND(VULKAN)
				gfx = new VulkanContext();
			#else
				assert(false && "Vulkan backend requested but not enabled in the gfx project");
			#endif
		}
		else if (gfxInit.gfxBackend == GfxContext::GfxBackend::D3D12) {
			#if VSV_GFX_BACKEND(D3D12)
				gfx = new VulkanContext();
			#else
				assert(false && "D3D12 backend requested but not enabled in the gfx project");
			#endif
		}
		assert(gfx);
		gfx->init(windowInit.name, window);
		gfxTest.init(gfx);
	}

	void Runtime::run() {
		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			updateFunc(0.0f);
			gfxTest.update();
			gfx->update();
		}
	}

	void Runtime::GfxTest::init(GfxContext* gfxContext) {
		gfx = gfxContext;
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

		vb = gfx->createVertexBuffer(vertices.data(), (uint16_t)vertices.size());
		ib = gfx->createIndexBuffer(indices.data(), (uint32_t)indices.size());
	}


	void Runtime::GfxTest::cleanup() {
		gfx->destroyVertexBuffer(vb);
		gfx->destroyIndexBuffer(ib);
	}

	void Runtime::GfxTest::update() {

	}

	void Runtime::framebufferResizeCallback(GLFWwindow* window, int width, int height) noexcept {
		Runtime* runtime = reinterpret_cast<Runtime*>(glfwGetWindowUserPointer(window));
		assert(runtime && runtime->gfx);
		runtime->gfx->resizeFramebuffer(static_cast<uint16_t>(width), static_cast<uint16_t>(height));
	}
}