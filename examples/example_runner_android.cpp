#include "example_runner_android.hpp"

#include <cstdio>
#include <unistd.h>
#include <android/log.h>
#include <__threading_support>

#include <volk.h>

namespace vuk {
    static bool begin_stdout_redirection(const char* app_name);

    ExampleRunnerAndroid::ExampleRunnerAndroid() {
        // begin_stdout_redirection("Stinky VUK Examples");

        create_instance();

        const auto surface_create_info = VkAndroidSurfaceCreateInfoKHR{
                .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
                .window = app->window
        };

        auto vk_result = vkCreateAndroidSurfaceKHR(vkbinstance.instance, &surface_create_info, nullptr,
                                                   &surface);
        if (vk_result != VK_SUCCESS) {
            throw std::runtime_error{"Could not create rendering surface"};
        }

        create_device();

        create_vuk();
    }

    void ExampleRunnerAndroid::setup_platform() {
        ImGui_ImplAndroid_Init(app->window);
    }

    void ExampleRunnerAndroid::set_window_title(std::string title) {

    }

    double ExampleRunnerAndroid::get_time() {
        return static_cast<double>(clock()) / CLOCKS_PER_SEC;
    }

    void ExampleRunnerAndroid::poll_events() {
        should_close = app->destroyRequested != 0;

        int events;
        android_poll_source* source;

        if (ALooper_pollAll(1, nullptr, &events, (void**) &source) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
        }

        ImGui_ImplAndroid_NewFrame();
    }

// from https://codelab.wordpress.com/2014/11/03/how-to-use-standard-output-streams-for-logging-in-android-apps/

    static int pfd[2];
    static pthread_t thr;
    static const char* tag = "myapp";

    static void* thread_func(void* dummy) {
        ssize_t rdsz;
        char buf[2048];
        while ((rdsz = read(pfd[0], buf, sizeof buf - 1)) > 0) {
            if (buf[rdsz - 1] == '\n') --rdsz;
            buf[rdsz] = 0;  /* add null-terminator */
            __android_log_write(ANDROID_LOG_DEBUG, tag, buf);
        }

        return nullptr;
    }

    bool begin_stdout_redirection(const char* app_name) {
        tag = app_name;

        /* make stdout line-buffered and stderr unbuffered */
        setvbuf(stdout, nullptr, _IOLBF, 0);
        setvbuf(stderr, nullptr, _IONBF, 0);

        /* create the pipe and redirect stdout and stderr */
        pipe(pfd);
        dup2(pfd[1], 1);
        dup2(pfd[1], 2);

        /* spawn the logging thread */
        if (pthread_create(&thr, 0, thread_func, 0) == -1) {
            return false;
        }
        pthread_detach(thr);
        return true;
    }
}
