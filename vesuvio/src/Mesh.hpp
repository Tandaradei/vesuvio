#pragma once

#include <vulkan/vulkan.hpp>

#include "Vertex.hpp"

namespace vesuvio {
	struct Mesh
	{

		const std::vector<Vertex> vertices;
		const std::vector<uint16_t> indices;

		const vk::Buffer buffer;
	};
}