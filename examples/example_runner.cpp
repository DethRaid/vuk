#include "example_runner.hpp"

#ifdef __ANDROID__
#else
#include "example_runner_glfw.hpp"
#endif

namespace vuk {
	void ExampleRunner::setup() {
		// Setup Dear ImGui context
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		// Setup Platform/Renderer bindings
		imgui_data = util::ImGui_ImplVuk_Init(*global);
		{
			std::vector<std::thread> threads;
			for (auto& ex : examples) {
				threads.emplace_back(std::thread([&] { ex->setup(*this, *global); }));
			}

			for (auto& thread : threads) {
				thread.join();
			}
		}

		setup_platform();
	}

	void ExampleRunner::cleanup() {
		context->wait_idle();
		for (auto& ex : examples) {
			if (ex->cleanup) {
				ex->cleanup(*this, *global);
			}
		}
	}

	ExampleRunner::~ExampleRunner() {
		present_ready.reset();
		render_complete.reset();
		imgui_data.font_texture.view.reset();
		imgui_data.font_texture.image.reset();
		xdev_rf_alloc.reset();
		context.reset();
		vkDestroySurfaceKHR(vkbinstance.instance, surface, nullptr);
		vkb::destroy_device(vkbdevice);
		vkb::destroy_instance(vkbinstance);
	}

	ExampleRunner& ExampleRunner::get_runner() {
#ifdef __ANDROID__
#else
		static std::unique_ptr<ExampleRunner> runner = std::make_unique<ExampleRunnerGlfw>();
#endif
		return *runner;
	}

	void ExampleRunner::create_instance() {
		vkb::InstanceBuilder builder;
		builder.request_validation_layers()
		    .set_debug_callback([](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		                           VkDebugUtilsMessageTypeFlagsEXT messageType,
		                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		                           void* pUserData) -> VkBool32 {
			    auto ms = vkb::to_string_message_severity(messageSeverity);
			    auto mt = vkb::to_string_message_type(messageType);
			    printf("[%s: %s](user defined)\n%s\n", ms, mt, pCallbackData->pMessage);
			    return VK_FALSE;
		    })
		    .set_app_name("vuk_example")
		    .set_engine_name("vuk")
		    .require_api_version(1, 2, 0)
		    .set_app_version(0, 1, 0);
		auto inst_ret = builder.build();
		if (!inst_ret) {
			throw std::runtime_error("Couldn't initialise instance");
		}

		has_rt = true;

		vkbinstance = inst_ret.value();
	}

	void ExampleRunner::create_device() {
		vkb::PhysicalDeviceSelector selector{ vkbinstance };
		selector.set_surface(surface)
		    .set_minimum_version(1, 0)
		    .add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME)
		    .add_required_extension(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
		    .add_required_extension(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
		    .add_required_extension(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
		auto phys_ret = selector.select();
		vkb::PhysicalDevice vkbphysical_device;
		if (!phys_ret) {
			has_rt = false;
			vkb::PhysicalDeviceSelector selector2{ vkbinstance };
			selector2.set_surface(surface).set_minimum_version(1, 0).add_required_extension(VK_KHR_SYNCHRONIZATION_2_EXTENSION_NAME);
			auto phys_ret2 = selector2.select();
			if (!phys_ret2) {
				throw std::runtime_error("Couldn't create physical device");
			} else {
				vkbphysical_device = phys_ret2.value();
			}
		} else {
			vkbphysical_device = phys_ret.value();
		}

		physical_device = vkbphysical_device.physical_device;
		vkb::DeviceBuilder device_builder{ vkbphysical_device };
		VkPhysicalDeviceVulkan12Features vk12features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		vk12features.timelineSemaphore = true;
		vk12features.descriptorBindingPartiallyBound = true;
		vk12features.descriptorBindingUpdateUnusedWhilePending = true;
		vk12features.shaderSampledImageArrayNonUniformIndexing = true;
		vk12features.runtimeDescriptorArray = true;
		vk12features.descriptorBindingVariableDescriptorCount = true;
		vk12features.hostQueryReset = true;
		vk12features.bufferDeviceAddress = true;
		vk12features.shaderOutputLayer = true;
		VkPhysicalDeviceVulkan11Features vk11features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES };
		vk11features.shaderDrawParameters = true;
		VkPhysicalDeviceFeatures2 vk10features{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR };
		vk10features.features.shaderInt64 = true;
		VkPhysicalDeviceSynchronization2FeaturesKHR sync_feat{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR,
			                                                     .synchronization2 = true };
		VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
			                                                             .accelerationStructure = true };
		VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{ .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
			                                                               .rayTracingPipeline = true };
		device_builder = device_builder.add_pNext(&vk12features).add_pNext(&vk11features).add_pNext(&sync_feat).add_pNext(&accelFeature).add_pNext(&vk10features);
		if (has_rt) {
			device_builder = device_builder.add_pNext(&rtPipelineFeature);
		}
		auto dev_ret = device_builder.build();
		if (!dev_ret) {
			throw std::runtime_error("Couldn't create device");
		}
		vkbdevice = dev_ret.value();
	}

	void ExampleRunner::create_vuk() {
		graphics_queue = vkbdevice.get_queue(vkb::QueueType::graphics).value();
		auto graphics_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::graphics).value();
		transfer_queue = vkbdevice.get_queue(vkb::QueueType::transfer).value();
		auto transfer_queue_family_index = vkbdevice.get_queue_index(vkb::QueueType::transfer).value();
		device = vkbdevice.device;
		ContextCreateParameters::FunctionPointers fps;
#define VUK_EX_LOAD_FP(name) fps.name = (PFN_##name)vkGetDeviceProcAddr(device, #name);
		if (has_rt) {
			VUK_EX_LOAD_FP(vkCmdBuildAccelerationStructuresKHR);
			VUK_EX_LOAD_FP(vkGetAccelerationStructureBuildSizesKHR);
			VUK_EX_LOAD_FP(vkCmdTraceRaysKHR);
			VUK_EX_LOAD_FP(vkCreateAccelerationStructureKHR);
			VUK_EX_LOAD_FP(vkDestroyAccelerationStructureKHR);
			VUK_EX_LOAD_FP(vkGetRayTracingShaderGroupHandlesKHR);
			VUK_EX_LOAD_FP(vkCreateRayTracingPipelinesKHR);
		}
		context.emplace(ContextCreateParameters{ vkbinstance.instance,
		                                         device,
		                                         physical_device,
		                                         graphics_queue,
		                                         graphics_queue_family_index,
		                                         VK_NULL_HANDLE,
		                                         VK_QUEUE_FAMILY_IGNORED,
		                                         transfer_queue,
		                                         transfer_queue_family_index,
		                                         fps });
		const unsigned num_inflight_frames = 3;
		xdev_rf_alloc.emplace(*context, num_inflight_frames);
		global.emplace(*xdev_rf_alloc);
		swapchain = context->add_swapchain(util::make_swapchain(vkbdevice));
		present_ready = vuk::Unique<std::array<VkSemaphore, 3>>(*global);
		render_complete = vuk::Unique<std::array<VkSemaphore, 3>>(*global);
	}
} // namespace vuk
