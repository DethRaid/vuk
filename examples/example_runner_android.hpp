#ifndef __ANDROID__
#error "Android runner is only supported on Android"
#endif

#include "example_runner.hpp"

#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "backends/imgui_impl_android.h"

namespace vuk {
	struct ExampleRunnerAndroid : ExampleRunner {
		static inline android_app* app;

		ExampleRunnerAndroid();

		void setup_platform() override;

		void set_window_title(std::string title) override;

		double get_time() override;

		void poll_events() override;

		~ExampleRunnerAndroid() = default;
	};
} // namespace vuk

