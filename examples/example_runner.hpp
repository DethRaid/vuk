#pragma once

#include "utils.hpp"
#include "vuk/Allocator.hpp"
#include "vuk/AllocatorHelpers.hpp"
#include "vuk/CommandBuffer.hpp"
#include "vuk/Context.hpp"
#include "vuk/Partials.hpp"
#include "vuk/RenderGraph.hpp"
#include "vuk/SampledImage.hpp"
#include "vuk/resources/DeviceFrameResource.hpp"
#include <VkBootstrap.h>
#include <functional>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <stdio.h>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace vuk {
	struct ExampleRunner;

	struct Example {
		std::string_view name;

		std::function<void(ExampleRunner&, vuk::Allocator&)> setup;
		std::function<vuk::Future(ExampleRunner&, vuk::Allocator&, vuk::Future)> render;
		std::function<void(ExampleRunner&, vuk::Allocator&)> cleanup;
	};

	struct ExampleRunner {
		VkDevice device;
		VkPhysicalDevice physical_device;
		VkQueue graphics_queue;
		VkQueue transfer_queue;
		std::optional<Context> context;
		std::optional<DeviceSuperFrameResource> xdev_rf_alloc;
		std::optional<Allocator> global;
		vuk::SwapchainRef swapchain;
		VkSurfaceKHR surface;
		vkb::Instance vkbinstance;
		vkb::Device vkbdevice;
		util::ImGuiData imgui_data;
		std::vector<Future> futures;
		std::mutex setup_lock;
		double old_time = 0;
		uint32_t num_frames = 0;
		bool has_rt;
		vuk::Unique<std::array<VkSemaphore, 3>> present_ready;
		vuk::Unique<std::array<VkSemaphore, 3>> render_complete;

		bool should_close = false;

		// when called during setup, enqueues a device-side operation to be completed before rendering begins
		void enqueue_setup(Future&& fut) {
			std::scoped_lock _(setup_lock);
			futures.emplace_back(std::move(fut));
		}

		plf::colony<vuk::SampledImage> sampled_images;
		std::vector<Example*> examples;

		ExampleRunner() = default;

		void setup();

		void render();

		void cleanup();

		virtual void setup_platform() = 0;

		virtual void set_window_title(std::string title) = 0;

		virtual double get_time() = 0;

		virtual void poll_events() = 0;

		virtual ~ExampleRunner();

		static ExampleRunner& get_runner();

	protected:
		// Child classes must call these create_ methods, and must create the VkSurface before calling create_device

		void create_instance();

		void create_device();

		void create_vuk();
	};
} // namespace vuk

namespace util {
	struct Register {
		Register(vuk::Example& x) {
			vuk::ExampleRunner::get_runner().examples.push_back(&x);
		}
	};
} // namespace util

#define CONCAT_IMPL(x, y)   x##y
#define MACRO_CONCAT(x, y)  CONCAT_IMPL(x, y)
#define REGISTER_EXAMPLE(x) util::Register MACRO_CONCAT(_reg_, __LINE__)(x)
