cmake_minimum_required(VERSION 3.7)
project(vuk-examples)

if(NOT VUK_USE_SHADERC)
    message(FATAL_ERROR "Building vuk examples require shaderc for building shaders, enable VUK_USE_SHADERC")
endif()

FetchContent_Declare(
  vk-bootstrap 
  GIT_REPOSITORY https://github.com/charles-lunarg/vk-bootstrap
  GIT_TAG        01de71738cd206eefe2537882888dc741881ec0b
)
FetchContent_MakeAvailable(vk-bootstrap)

if(NOT ANDROID)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
      glfw 
      GIT_REPOSITORY https://github.com/glfw/glfw
      GIT_TAG        3.3.6
    )
    FetchContent_MakeAvailable(glfw)
endif()

 # VOLK party    
 FetchContent_Declare(
         fetch_volk
         GIT_REPOSITORY  https://github.com/zeux/volk.git
         GIT_TAG         3872818bb956e3b1e3847f3ddbb467b181e6a864
 )
 FetchContent_MakeAvailable(fetch_volk)

 message(STATUS "Downloaded volk")

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG        cc98465e3508535ba8c7f6208df934c156a018dc
)
FetchContent_MakeAvailable(glm)

SET(imgui_sources 
    ../ext/imgui/imgui.cpp 
    ../ext/imgui/imgui_draw.cpp 
    ../ext/imgui/imgui_demo.cpp 
    ../ext/imgui/imgui_widgets.cpp 
    ../ext/imgui/imgui_tables.cpp
)

if(NOT ANDROID)
    set(platform_sources
            ../ext/imgui/backends/imgui_impl_glfw.cpp
            )
else()
    set(platform_sources
            ../ext/imgui/backends/imgui_impl_android.cpp
            android_game_activity_source.cpp
            example_runner_android.cpp
            )
endif()

add_library(vuk_all_examples SHARED)
# add_executable(vuk_all_examples)

target_include_directories(vuk_all_examples SYSTEM PRIVATE ../ext/stb ../ext/imgui)
target_sources(vuk_all_examples PRIVATE 
    example_runner.cpp 
    example_browser.cpp 
    volk_implementation.cpp
    imgui.cpp 
    stbi.cpp 
    ${imgui_sources}
    ${platform_sources}
)

target_compile_definitions(vuk_all_examples PRIVATE 
    GLM_FORCE_SIZE_FUNC 
    GLM_FORCE_EXPLICIT_CTOR 
    GLM_ENABLE_EXPERIMENTAL 
    GLM_FORCE_RADIANS 
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    VK_NO_PROTOTYPES 
)
    
target_compile_definitions(vuk PUBLIC VUK_CUSTOM_VULKAN_HEADER=<volk.h>)
cmake_policy(SET CMP0079 NEW)
target_link_libraries(vuk PUBLIC volk_headers)

target_link_libraries(vuk_all_examples PRIVATE vuk)
target_link_libraries(vuk_all_examples PRIVATE vk-bootstrap glm volk_headers)
set_target_properties(vuk_all_examples
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
)
if(VUK_COMPILER_CLANGPP OR VUK_COMPILER_GPP)
    target_compile_options(vuk_all_examples PRIVATE -std=c++20 -fno-char8_t)
elseif(MSVC)
    target_compile_options(vuk_all_examples PRIVATE /std:c++latest /permissive- /Zc:char8_t-)
endif()

if(NOT ANDROID) 
    target_link_libraries(vuk_all_examples PRIVATE glfw)
elseif(ANDROID)
    # Integrate GameActivity, refer to
    #     https://d.android.com/games/agdk/integrate-game-activity
    # for the detailed instructions.
    find_package(game-activity REQUIRED CONFIG)

    target_compile_definitions(vuk_all_examples PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
    target_compile_definitions(vuk PRIVATE VK_USE_PLATFORM_ANDROID_KHR)

    target_link_libraries(vuk_all_examples PRIVATE
        game-activity::game-activity
        log
        android
    )
endif()

function(ADD_EXAMPLE name)
    set(FULL_NAME "vuk_example_${name}")
    target_sources(vuk_all_examples PRIVATE "${name}.cpp")

    if(NOT ANDROID)
        add_executable(${FULL_NAME})
        target_sources(${FULL_NAME} PRIVATE 
            "${name}.cpp" 
            example_runner.cpp 
            example_runner_single.cpp 
            volk_implementation.cpp
            imgui.cpp 
            stbi.cpp 
            ${imgui_sources}
        )
        
        target_include_directories(${FULL_NAME} SYSTEM PRIVATE ../ext/stb ../ext/imgui)
        target_compile_definitions(${FULL_NAME} PRIVATE 
            GLM_FORCE_SIZE_FUNC 
            GLM_FORCE_EXPLICIT_CTOR 
            GLM_ENABLE_EXPERIMENTAL 
            GLM_FORCE_RADIANS 
            GLM_FORCE_DEPTH_ZERO_TO_ONE 
            VK_NO_PROTOTYPES
        )
        target_link_libraries(${FULL_NAME} PRIVATE vuk)
        target_link_libraries(${FULL_NAME} PRIVATE vk-bootstrap glfw glm)
        set_target_properties(${FULL_NAME}
            PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}"
        )
        if(VUK_COMPILER_CLANGPP OR VUK_COMPILER_GPP)
	        target_compile_options(${FULL_NAME} PRIVATE -std=c++20 -fno-char8_t)
        elseif(MSVC)
	        target_compile_options(${FULL_NAME} PRIVATE /std:c++latest /permissive- /Zc:char8_t-)
        endif()
    endif()
endfunction(ADD_EXAMPLE)

ADD_EXAMPLE(01_triangle)
ADD_EXAMPLE(02_cube)
ADD_EXAMPLE(03_multipass)
ADD_EXAMPLE(04_texture)
ADD_EXAMPLE(05_deferred)
ADD_EXAMPLE(06_msaa)
ADD_EXAMPLE(07_commands)
ADD_EXAMPLE(08_pipelined_compute)
ADD_EXAMPLE(09_persistent_descriptorset)
ADD_EXAMPLE(10_baby_renderer)
ADD_EXAMPLE(11_composition)

if(NOT ANDROID)
    ADD_EXAMPLE(12_rt_pipeline)
endif()

message(STATUS " Added examples")
