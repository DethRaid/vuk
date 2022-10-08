#ifdef __ANDROID__
#error "GLFW runner is not supported on Android"
#endif

#include "example_runner.hpp"

#include "backends/imgui_impl_glfw.h"
#include "glfw.hpp"

namespace vuk {
	struct ExampleRunnerGlfw : ExampleRunner {
		GLFWwindow* window;

		ExampleRunnerGlfw() {
			create_instance();

			window = create_window_glfw("Vuk example", false);
			surface = create_surface_glfw(vkbinstance.instance, window);

			create_device();

			create_vuk();
		}
		
		void setup_platform() override {
			ImGui_ImplGlfw_InitForVulkan(window, true);

			glfwSetWindowSizeCallback(window, [](GLFWwindow* window, int width, int height) {

			});
		}

		void set_window_title(std::string title) override {
			glfwSetWindowTitle(window, title.c_str());
		}

		double get_time() override {
			return glfwGetTime();
		}

		void poll_events() override {
			should_close = glfwWindowShouldClose(window);
			glfwPollEvents();
			ImGui_ImplGlfw_NewFrame();
		}

		~ExampleRunnerGlfw() override {
			destroy_window_glfw(window);
		}
	};
} // namespace vuk
