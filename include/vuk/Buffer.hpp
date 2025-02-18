#pragma once

#include "Types.hpp"
#include <assert.h>

namespace vuk {
	enum class BufferUsageFlagBits : VkBufferUsageFlags {
		eTransferRead = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		eTransferWrite = VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		eUniformTexelBuffer = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
		eStorageTexelBuffer = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
		eUniformBuffer = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		eStorageBuffer = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		eIndexBuffer = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		eVertexBuffer = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		eIndirectBuffer = VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		eShaderDeviceAddress = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
		eTransformFeedbackBufferEXT = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT,
		eTransformFeedbackCounterBufferEXT = VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT,
		eConditionalRenderingEXT = VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT,
		eShaderDeviceAddressEXT = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_EXT,
		eShaderDeviceAddressKHR = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT_KHR,
		eAccelerationStructureBuildInputReadOnlyKHR = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR,
		eAccelerationStructureStorageKHR = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR,
		eShaderBindingTable = VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR
	};

	using BufferUsageFlags = Flags<BufferUsageFlagBits>;

	inline constexpr BufferUsageFlags operator|(BufferUsageFlagBits bit0, BufferUsageFlagBits bit1) noexcept {
		return BufferUsageFlags(bit0) | bit1;
	}

	inline constexpr BufferUsageFlags operator&(BufferUsageFlagBits bit0, BufferUsageFlagBits bit1) noexcept {
		return BufferUsageFlags(bit0) & bit1;
	}

	inline constexpr BufferUsageFlags operator^(BufferUsageFlagBits bit0, BufferUsageFlagBits bit1) noexcept {
		return BufferUsageFlags(bit0) ^ bit1;
	}

	/// @brief A contiguous portion of GPU-visible memory that can be used for storing buffer-type data
	struct Buffer {
		VkBuffer buffer = VK_NULL_HANDLE;
		size_t offset = 0;
		size_t size = ~(0u);
		size_t allocation_size = 0;
		uint64_t device_address = 0;
		std::byte* mapped_ptr = nullptr;
		MemoryUsage memory_usage;

		bool operator==(const Buffer& o) const noexcept {
			return buffer == o.buffer && offset == o.offset && size == o.size;
		}

		explicit operator bool() const noexcept {
			return buffer != VK_NULL_HANDLE;
		}

		/// @brief Create a new Buffer by offsetting
		[[nodiscard]] Buffer add_offset(size_t offset_to_add) {
			assert(offset_to_add <= size);
			return { buffer,
				       offset + offset_to_add,
				       size - offset_to_add,
				       allocation_size,
				       device_address != 0 ? device_address + offset_to_add : 0,
				       mapped_ptr != nullptr ? mapped_ptr + offset_to_add : nullptr,
				       memory_usage };
		}

		/// @brief Create a new Buffer that is a subset of the original
		[[nodiscard]] Buffer subrange(size_t new_offset, size_t new_size) {
			assert(new_offset + new_size <= size);
			return { buffer,
				       offset + new_offset,
				       new_size,
				       allocation_size,
				       device_address != 0 ? device_address + new_offset : 0,
				       mapped_ptr != nullptr ? mapped_ptr + new_offset : nullptr,
				       memory_usage };
		}
	};

	/// @brief A buffer that has GPU-only scope (only accessed from the device timeline)
	struct BufferGPU : Buffer {};
	/// @brief A buffer with cross-device scope (accessed both from device and host timeline)
	struct BufferCrossDevice : Buffer {};

	/// @brief Buffer creation parameters
	struct BufferCreateInfo {
		/// @brief Memory usage to determine which heap to allocate the memory from
		MemoryUsage mem_usage;
		/// @brief Size of the Buffer in bytes
		size_t size;
		/// @brief Alignment of the allocated Buffer in bytes
		size_t alignment = 1;
	};
} // namespace vuk
