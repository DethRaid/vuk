#include "example_runner.hpp"

#ifdef __ANDROID__
#include <android/log.h>
#include <game-activity/native_app_glue/android_native_app_glue.h>

#include "example_runner_android.hpp"
#endif

void vuk::ExampleRunner::render() {
	Compiler compiler;
	vuk::wait_for_futures_explicit(*global, compiler, futures);
	futures.clear();

	while (!should_close ) {
		poll_events();
		auto& xdev_frame_resource = xdev_rf_alloc->get_next_frame();
		context->next_frame();
		Allocator frame_allocator(xdev_frame_resource);
		RenderGraph rg("runner");
		auto attachment_name = vuk::Name(examples[0]->name);
		rg.attach_swapchain("_swp", swapchain);
		rg.clear_image("_swp", attachment_name, vuk::ClearColor{ 0.3f, 0.5f, 0.3f, 1.0f });
		auto fut = examples[0]->render(*this, frame_allocator, Future{ std::make_shared<RenderGraph>(std::move(rg)), attachment_name });
		present(frame_allocator, compiler, swapchain, std::move(fut));
		if (++num_frames == 16) {
			auto new_time = get_time();
			auto delta = new_time - old_time;
			auto per_frame_time = delta / 16 * 1000;
			old_time = new_time;
			num_frames = 0;
			set_window_title(std::string("Vuk example browser [") + std::to_string(per_frame_time) + " ms / " + std::to_string(1000 / per_frame_time) + " FPS]");
		}
	}
}

#ifdef __ANDROID__
void android_main(struct android_app* app) {
	// Set the callback to process system events
	app->onAppCmd = nullptr;

	vuk::ExampleRunnerAndroid::app = app;

	// Used to poll the events in the main loop
	int events;
	android_poll_source* source;

	vuk::ExampleRunner::get_runner().setup();
	vuk::ExampleRunner::get_runner().render();
	vuk::ExampleRunner::get_runner().cleanup();
}

#else
int main() {
	vuk::ExampleRunner::get_runner().setup();
	vuk::ExampleRunner::get_runner().render();
	vuk::ExampleRunner::get_runner().cleanup();
}
#endif
