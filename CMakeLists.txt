cmake_minimum_required(VERSION 3.31)

project(PrimordialEngine
  VERSION 0.1.0
  DESCRIPTION "Game / Rendering Engine"
  LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# Includes
# ==============================================================================
include(FetchContent)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(CompileShaders)
include(FormatProject)

# ==============================================================================
# Defaults
# ==============================================================================
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES Debug CACHE STRING "Choose the build type")
  message(WARNING "No build type specified. Defaulting to 'Debug'.")
endif()

# ==============================================================================
# Dependencies
# ==============================================================================
FetchContent_Declare(glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.5.1
    GIT_SHALLOW TRUE
    SYSTEM
    FIND_PACKAGE_ARGS NAMES glfw glfw3
)
set(GLFW_BUILD_DOCS OFF)

FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.3
    GIT_SHALLOW TRUE
    SYSTEM
    FIND_PACKAGE_ARGS
)
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG docking
    GIT_SHALLOW TRUE
    SYSTEM
)

set(PE_VULKAN_SDK_VERSION 1.4.335)
FetchContent_Declare(VulkanHeaders
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Headers.git
    GIT_TAG "vulkan-sdk-${PE_VULKAN_SDK_VERSION}"
    GIT_SHALLOW TRUE
    SYSTEM
    FIND_PACKAGE_ARGS
)
FetchContent_Declare(VulkanLoader
    GIT_REPOSITORY https://github.com/KhronosGroup/Vulkan-Loader.git
    GIT_TAG "vulkan-sdk-${PE_VULKAN_SDK_VERSION}"
    GIT_SHALLOW TRUE
    SYSTEM
    FIND_PACKAGE_ARGS
)

FetchContent_MakeAvailable(VulkanHeaders VulkanLoader glfw glm imgui)

# ==============================================================================
# Targets & Sources
# ==============================================================================
# Vulkan
add_library(Vulkan::Vulkan INTERFACE IMPORTED GLOBAL)

# ImGui
add_library(ImGui STATIC)
target_sources(ImGui
    PRIVATE
      ${imgui_SOURCE_DIR}/imgui.cpp
      ${imgui_SOURCE_DIR}/imgui_demo.cpp
      ${imgui_SOURCE_DIR}/imgui_draw.cpp
      ${imgui_SOURCE_DIR}/imgui_tables.cpp
      ${imgui_SOURCE_DIR}/imgui_widgets.cpp
      ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
      ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
      ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp

    PUBLIC
      FILE_SET HEADERS
      BASE_DIRS
        ${imgui_SOURCE_DIR}
      FILES
        ${imgui_SOURCE_DIR}/imgui.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.h
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.h
)

# Primordial Engine
file(GLOB_RECURSE PE_SOURCE_FILES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB_RECURSE PE_HEADER_FILES CONFIGURE_DEPENDS "include/*.h")
file(GLOB_RECURSE VENDOR_HEADERS CONFIGURE_DEPENDS "src/Vendor/*.h")
add_executable(PrimordialEngine)
target_sources(PrimordialEngine
    PRIVATE
      ${PE_SOURCE_FILES}

    PRIVATE
      FILE_SET HEADERS
      BASE_DIRS
        include
        src/Vendor
      FILES
        ${PE_HEADER_FILES}
        ${VENDOR_HEADERS}
)

# ==============================================================================
# Linking
# ==============================================================================
target_link_libraries(Vulkan::Vulkan INTERFACE Vulkan::Headers Vulkan::Loader)
target_link_libraries(ImGui PRIVATE glfw Vulkan::Vulkan)
if(WIN32)
  target_link_libraries(PrimordialEngine PRIVATE
      glfw
      Vulkan::Vulkan
      glm::glm
      ImGui
)
elseif(UNIX)
  target_link_libraries(PrimordialEngine PRIVATE
      glfw
      Vulkan::Vulkan
      glm::glm
      ImGui
)
endif()

# ==============================================================================
# Features & Definitions
# ==============================================================================
target_compile_features(PrimordialEngine PRIVATE cxx_std_20)
target_compile_definitions(PrimordialEngine PRIVATE
    GLFW_INCLUDE_VULKAN
    GLM_FORCE_LEFT_HANDED
    GLM_FORCE_DEPTH_ZERO_TO_ONE
    GLM_ENABLE_EXPERIMENTAL
    GLM_FORCE_RADIANS
    GLM_FORCE_SSE2
    PE_VULKAN # Vulkan is the only supported graphic backend for now.
)
if(WIN32)
  target_compile_definitions(PrimordialEngine PRIVATE
    _UNICODE
    _HAS_STD_BYTE=0
    GLFW_EXPOSE_NATIVE_WIN32
    UNICODE
    VK_USE_PLATFORM_WIN32_KHR
    WIN32_LEAN_AND_MEAN
)
endif()

# ==============================================================================
# Developer Tools & Quality Checks
# ==============================================================================
PE_FormatProject()

# ==============================================================================
# Build
# ==============================================================================
PE_CompileShaders(PrimordialEngine "${CMAKE_CURRENT_SOURCE_DIR}/shaders")

add_custom_command(TARGET PrimordialEngine POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${CMAKE_CURRENT_SOURCE_DIR}/assets"
      "$<TARGET_FILE_DIR:PrimordialEngine>/assets"
)

# ==============================================================================
# Install
# ==============================================================================
install(TARGETS PrimordialEngine RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/assets" DESTINATION .)
install(DIRECTORY "$<TARGET_FILE_DIR:PrimordialEngine>/shaders" DESTINATION .)
