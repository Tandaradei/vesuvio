#pragma once
#include <stdint.h>
#include <functional>

#define VSV_ENABLE_VULKAN
#include "vesuvio/gfx/GfxContext.hpp"

struct GLFWwindow;

namespace vesuvio {
	class Runtime
	{
	public:
		using UpdateFunc = std::function<void(float frametime)>;
		struct WindowInit
		{
			const char* name;
			uint16_t width;
			uint16_t height;
		};

		struct GfxInit
		{
			GfxContext::GfxBackend gfxBackend;
		};

	public:
		Runtime();
		~Runtime();
		void init(WindowInit windowInit, GfxInit gfxInit, UpdateFunc updateFn);
		void run();

	private:
		struct GfxTest
		{
			void init(GfxContext* gfxContext);
			void cleanup();
			void update();
			GfxContext* gfx;

			VertexBuffer* vb;
			IndexBuffer* ib;
		} gfxTest;
		static void framebufferResizeCallback(GLFWwindow* window, int width, int height) noexcept;
	private:
		UpdateFunc updateFunc;
		GLFWwindow* window;
		GfxContext* gfx;
	};
}