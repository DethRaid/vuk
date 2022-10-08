#if defined( __ANDROID__)
#define VK_PLATFORM_ANDROID
#elif defined( _WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#else
#define VK_USE_PLATFORM_XLIB_KHR
#endif

#define VOLK_IMPLEMENTATION
#include <volk.h>
