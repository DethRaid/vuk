#pragma once

#include "../src/CreateInfo.hpp"
#include "../src/ToIntegral.hpp"

#include "vuk/Buffer.hpp"
#include "vuk/Hash.hpp"
#include "vuk/Image.hpp"
#include "vuk/Types.hpp"

#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <string.h>
#include <unordered_map>
#include <utility>
#include <vk_mem_alloc.h>

namespace vuk {
	struct PoolSelect {
		MemoryUsage mem_usage;
		vuk::BufferUsageFlags buffer_usage;

		bool operator==(const PoolSelect& o) const noexcept {
			return std::tie(mem_usage, buffer_usage) == std::tie(o.mem_usage, o.buffer_usage);
		}
	};

	struct BufferID {
		uint64_t vk_buffer;
		uint64_t offset;

		bool operator==(const BufferID& o) const noexcept {
			return ::memcmp(this, &o, sizeof(BufferID)) == 0;
		}
	};

}; // namespace vuk

namespace std {
	template<>
	struct hash<vuk::PoolSelect> {
		size_t operator()(vuk::PoolSelect const& x) const noexcept {
			size_t h = 0;
			hash_combine(h, to_integral(x.mem_usage), x.buffer_usage);
			return h;
		}
	};

	template<>
	struct hash<vuk::BufferID> {
		size_t operator()(vuk::BufferID const& x) const noexcept {
			size_t h = 0;
			hash_combine(h, x.vk_buffer, x.offset);
			return h;
		}
	};
}; // namespace std

namespace vuk {
	struct LegacyPoolAllocator {
		VmaPool pool;
		VkMemoryRequirements mem_reqs;
		vuk::MemoryUsage mem_usage;
		vuk::BufferUsageFlags usage;
		std::vector<VkBuffer> buffers;
	};

	struct LinearAllocSegment {
		VmaAllocation allocation = nullptr;
		VkDeviceMemory device_memory;
		size_t device_memory_offset;
		VkBuffer buffer = VK_NULL_HANDLE;
		std::byte* mapped_ptr = nullptr;
		uint64_t bda;
		size_t num_blocks;
		uint64_t base_address = 0;
	};

	struct LegacyLinearAllocator {
		std::atomic<int> current_buffer = -1;
		std::atomic<uint64_t> needle = 0;
		VkMemoryRequirements mem_reqs;
		VmaMemoryUsage mem_usage;
		vuk::BufferUsageFlags usage;
		std::array<LinearAllocSegment, 256> available_allocations; // up to 4 GB of allocations
		std::array<LinearAllocSegment, 256> used_allocations; // up to 4 GB of allocations
		size_t available_allocation_count = 0;
		size_t used_allocation_count = 0;

		size_t block_size = 1024 * 1024 * 16;

		LegacyLinearAllocator(VkMemoryRequirements mem_reqs, VmaMemoryUsage mem_usage, vuk::BufferUsageFlags buf_usage) :
		    mem_reqs(mem_reqs),
		    mem_usage(mem_usage),
		    usage(buf_usage) {}

		LegacyLinearAllocator(LegacyLinearAllocator&& o) noexcept {
			current_buffer = o.current_buffer.load();
			needle = o.needle.load();
			mem_reqs = o.mem_reqs;
			mem_usage = o.mem_usage;
			usage = o.usage;
			used_allocations = o.used_allocations;
			available_allocations = o.available_allocations;
			block_size = o.block_size;
			available_allocation_count = o.available_allocation_count;
			used_allocation_count = o.used_allocation_count;
		}
	};

	class LegacyGPUAllocator {
		std::mutex mutex;
		struct PoolAllocHelper {
			VkDevice device;
			VkBufferCreateInfo bci;
			VkBuffer result;
			uint64_t device_address;
			PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectNameEXT;
		};
		std::unique_ptr<PoolAllocHelper> pool_helper;

		static void pool_cb(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* userdata);

		static void noop_cb(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* userdata);

		static PFN_vmaAllocateDeviceMemoryFunction real_alloc_callback;

		static void allocation_cb(VmaAllocator allocator, uint32_t memoryType, VkDeviceMemory memory, VkDeviceSize size, void* userdata) {
			real_alloc_callback(allocator, memoryType, memory, size, userdata);
		}
		VkDevice device;

		std::unordered_map<uint64_t, VmaAllocation> images;
		std::unordered_map<BufferID, VmaAllocation> buffer_allocations;
		std::unordered_map<PoolSelect, LegacyPoolAllocator> pools;
		std::unordered_map<uint64_t, std::tuple<VkBuffer, size_t, uint64_t>> buffers;

		VmaAllocator allocator;
		VkPhysicalDeviceProperties properties;
		std::vector<uint32_t> all_queue_families;
		uint32_t queue_family_count;

	public:
		LegacyGPUAllocator(VkInstance instance,
		                   VkDevice device,
		                   VkPhysicalDevice phys_dev,
		                   uint32_t graphics_queue_family,
		                   uint32_t compute_queue_family,
		                   uint32_t transfer_queue_family);
		~LegacyGPUAllocator();

		// allocate an externally managed pool
		LegacyPoolAllocator allocate_pool(MemoryUsage mem_usage, vuk::BufferUsageFlags buffer_usage);
		// allocate an externally managed linear pool
		static constexpr vuk::BufferUsageFlags all_usage =
		    BufferUsageFlagBits::eTransferRead | BufferUsageFlagBits::eTransferWrite | BufferUsageFlagBits::eUniformTexelBuffer |
		    BufferUsageFlagBits::eStorageTexelBuffer | BufferUsageFlagBits::eUniformBuffer | BufferUsageFlagBits::eStorageBuffer |
		    BufferUsageFlagBits::eIndexBuffer | BufferUsageFlagBits::eVertexBuffer | BufferUsageFlagBits::eIndirectBuffer |
		    BufferUsageFlagBits::eShaderDeviceAddress | BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
		    BufferUsageFlagBits::eAccelerationStructureStorageKHR | BufferUsageFlagBits::eShaderBindingTable;

		static constexpr size_t large_allocation_size = 256 * 1024 * 1024;

		VkMemoryRequirements get_memory_requirements(VkBufferCreateInfo& bci);
		LegacyLinearAllocator allocate_linear(MemoryUsage mem_usage, vuk::BufferUsageFlags buffer_usage);
		// allocate buffer from an internally managed pool
		Buffer allocate_buffer(MemoryUsage mem_usage, vuk::BufferUsageFlags buffer_usage, size_t size, size_t alignment, bool create_mapped);
		// allocate a buffer from an externally managed pool
		Buffer allocate_buffer(LegacyPoolAllocator& pool, size_t size, size_t alignment, bool create_mapped);
		// allocate a buffer from an externally managed linear pool
		Buffer allocate_buffer(LegacyLinearAllocator& pool, size_t size, size_t alignment, bool create_mapped);

		size_t get_allocation_size(const Buffer&);

		void reset_pool(LegacyPoolAllocator& pool);
		void reset_pool(LegacyLinearAllocator& pool);
		// trims used buffers to current usage
		void trim_pool(LegacyLinearAllocator& pool);

		void free_buffer(const Buffer& b);
		void destroy(const LegacyPoolAllocator& pool);
		void destroy(const LegacyLinearAllocator& pool);

		vuk::Image create_image_for_rendertarget(vuk::ImageCreateInfo ici);
		vuk::Image create_image(vuk::ImageCreateInfo ici);
		void destroy_image(vuk::Image image);

	private:
		// not locked, must be called from a locked fn
		VmaPool _create_pool(MemoryUsage mem_usage, vuk::BufferUsageFlags buffer_usage);
		Buffer _allocate_buffer(LegacyPoolAllocator& pool, size_t size, size_t alignment, bool create_mapped);
		Buffer _allocate_buffer(LegacyLinearAllocator& pool, size_t size, size_t alignment, bool create_mapped);
		void _grow(LegacyLinearAllocator& pool, size_t num_pool_segs);
	};

	template<>
	struct create_info<LegacyPoolAllocator> {
		using type = PoolSelect;
	};

	template<>
	struct create_info<LegacyLinearAllocator> {
		using type = PoolSelect;
	};
}; // namespace vuk
