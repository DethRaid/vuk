#ifndef __ANDROID__
#error "Android runner is only supported on Android"
#endif

#include "example_runner.hpp"

#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "backends/imgui_impl_android.h"

namespace vuk {
	struct ExampleRunnerAndroid : ExampleRunner {
		static android_app* app;

		ExampleRunnerAndroid() {
			create_instance();

			surface = vkbdevice.surface;

			create_device();

			create_vuk();
		}

		void setup_platform() override {
			ImGui_ImplAndroid_Init(app->window);
		}

		void set_window_title(std::string title) override {}

		double get_time() override {
			return static_cast<double>(clock()) / CLOCKS_PER_SEC;
		}

		void poll_events() override {
			should_close = app->destroyRequested != 0;

			int events;
			android_poll_source* source;

			if (ALooper_pollAll(1, nullptr, &events, (void**)&source) >= 0) {
				if (source != nullptr) {
					source->process(app, source);
				}
			}

			ImGui_ImplAndroid_NewFrame();
		}

		~ExampleRunnerAndroid() = default;
	};
} // namespace vuk

