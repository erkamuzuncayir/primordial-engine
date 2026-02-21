cmake_minimum_required(VERSION 3.20)

project(PrimordialEngine
        VERSION 0.1.0
        DESCRIPTION "High-Performance ECS Rendering Engine"
        LANGUAGES CXX C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the build type" FORCE)
endif()

# ==============================================================================
# Dependencies
# ==============================================================================
include(FetchContent)

FetchContent_Declare(glfw GIT_REPOSITORY https://github.com/glfw/glfw.git GIT_TAG 3.4)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG 1.0.2)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui GIT_TAG docking)

FetchContent_MakeAvailable(glfw glm imgui)

find_package(Vulkan REQUIRED)

# ==============================================================================
# Shader Compilation Pipeline
# ==============================================================================
function(pe_compile_shaders TARGET_NAME SHADER_INPUT_DIR SHADER_OUTPUT_DIR)
    set(COMPILED_SHADERS "")
    file(MAKE_DIRECTORY "${SHADER_OUTPUT_DIR}")

    # --- HLSL (DirectX) ---
    if(WIN32)
        find_program(FXC_COMPILER "fxc.exe" PATHS
                "C:/Program Files (x86)/Windows Kits/10/bin/x64"
                "C:/Program Files (x86)/Windows Kits/8.1/bin/x64"
        )
        if(FXC_COMPILER)
            file(GLOB_RECURSE HLSL_FILES "${SHADER_INPUT_DIR}/*.hlsl")
            foreach(SHADER_FILE ${HLSL_FILES})
                get_filename_component(FILENAME ${SHADER_FILE} NAME_WE)

                set(VS_OUT "${SHADER_OUTPUT_DIR}/${FILENAME}_vs.cso")
                add_custom_command(
                        OUTPUT ${VS_OUT}
                        COMMAND ${FXC_COMPILER} /nologo /T vs_5_0 /E VSMain /Fo ${VS_OUT} ${SHADER_FILE}
                        DEPENDS ${SHADER_FILE}
                )
                list(APPEND COMPILED_SHADERS ${VS_OUT})

                set(PS_OUT "${SHADER_OUTPUT_DIR}/${FILENAME}_ps.cso")
                add_custom_command(
                        OUTPUT ${PS_OUT}
                        COMMAND ${FXC_COMPILER} /nologo /T ps_5_0 /E PSMain /Fo ${PS_OUT} ${SHADER_FILE}
                        DEPENDS ${SHADER_FILE}
                )
                list(APPEND COMPILED_SHADERS ${PS_OUT})
            endforeach()
        endif()
    endif()

    # --- GLSL (Vulkan) ---
    find_program(GLSLC_EXECUTABLE NAMES glslc HINTS "$ENV{VULKAN_SDK}/Bin")
    if(GLSLC_EXECUTABLE)
        file(GLOB_RECURSE GLSL_FILES "${SHADER_INPUT_DIR}/*.glsl")
        foreach(SHADER_FILE ${GLSL_FILES})
            get_filename_component(FILENAME ${SHADER_FILE} NAME_WE)

            set(VERT_OUT "${SHADER_OUTPUT_DIR}/${FILENAME}_vert.spv")
            add_custom_command(
                    OUTPUT ${VERT_OUT}
                    COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=vert -DVERTEX_SHADER -o ${VERT_OUT} ${SHADER_FILE}
                    DEPENDS ${SHADER_FILE}
            )
            list(APPEND COMPILED_SHADERS ${VERT_OUT})

            set(FRAG_OUT "${SHADER_OUTPUT_DIR}/${FILENAME}_frag.spv")
            add_custom_command(
                    OUTPUT ${FRAG_OUT}
                    COMMAND ${GLSLC_EXECUTABLE} -fshader-stage=frag -DFRAGMENT_SHADER -o ${FRAG_OUT} ${SHADER_FILE}
                    DEPENDS ${SHADER_FILE}
            )
            list(APPEND COMPILED_SHADERS ${FRAG_OUT})
        endforeach()
    endif()

    if(COMPILED_SHADERS)
        add_custom_target(${TARGET_NAME}_Shaders ALL DEPENDS ${COMPILED_SHADERS})
        add_dependencies(${TARGET_NAME} ${TARGET_NAME}_Shaders)
    endif()
endfunction()

# ==============================================================================
# Sources & Target
# ==============================================================================
file(GLOB_RECURSE ENGINE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE ENGINE_HEADERS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h")

set(IMGUI_SOURCES
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
)

if(WIN32)
    list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp)
endif()

add_executable(PrimordialEngine ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${IMGUI_SOURCES})

target_compile_features(PrimordialEngine PUBLIC cxx_std_20)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/src" PREFIX "Source Files" FILES ${ENGINE_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/include" PREFIX "Header Files" FILES ${ENGINE_HEADERS})
source_group("Vendor/ImGui" FILES ${IMGUI_SOURCES})

# ==============================================================================
# Code Formatting (clang-format)
# ==============================================================================
list(FILTER ENGINE_SOURCES EXCLUDE REGEX ".*/src/Vendor/.*")
list(FILTER ENGINE_HEADERS EXCLUDE REGEX ".*/src/Vendor/.*")

find_program(CLANG_FORMAT_EXE clang-format)

if(CLANG_FORMAT_EXE)
    add_custom_target(format
            COMMAND ${CLANG_FORMAT_EXE} -style=file -i ${ENGINE_SOURCES} ${ENGINE_HEADERS}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            COMMENT "Running clang-format on project's source and header files..."
    )
else()
    message(WARNING "clang-format executable not found. The 'format' target is disabled.")
endif()

# ==============================================================================
# Configuration & Linking
# ==============================================================================
target_include_directories(PrimordialEngine PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/lib"
        ${Vulkan_INCLUDE_DIRS}
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
)

target_include_directories(PrimordialEngine SYSTEM PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Vendor"
)

target_compile_definitions(PrimordialEngine PRIVATE
        GLM_FORCE_LEFT_HANDED
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_ENABLE_EXPERIMENTAL
        GLM_FORCE_RADIANS
        GLM_FORCE_SSE2
        PE_VULKAN
)

if(WIN32)
    target_compile_definitions(PrimordialEngine PRIVATE WIN32_LEAN_AND_MEAN UNICODE _UNICODE _HAS_STD_BYTE=0)
    target_link_libraries(PrimordialEngine PRIVATE glfw glm::glm Vulkan::Vulkan d3d11 dxgi d3dcompiler)
elseif(UNIX)
    target_link_libraries(PrimordialEngine PRIVATE glfw glm::glm Vulkan::Vulkan dl)
endif()

# ==============================================================================
# Asset Deployment (Intermediate -> Final)
# ==============================================================================
set(INTERMEDIATE_SHADERS_DIR "${CMAKE_BINARY_DIR}/compiled_shaders")
set(FINAL_ASSETS_DIR "$<TARGET_FILE_DIR:PrimordialEngine>/assets")

# 1. Compile to Intermediate Directory (Build Tree)
pe_compile_shaders(PrimordialEngine "${CMAKE_CURRENT_SOURCE_DIR}/assets" "${INTERMEDIATE_SHADERS_DIR}")

# 2. Deploy Logic
add_custom_command(TARGET PrimordialEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${FINAL_ASSETS_DIR}"
        COMMENT "Ensuring assets directory exists"
)

add_custom_command(TARGET PrimordialEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${FINAL_ASSETS_DIR}"
        COMMENT "Deploying raw assets"
)

add_custom_command(TARGET PrimordialEngine POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${INTERMEDIATE_SHADERS_DIR}"
        "${FINAL_ASSETS_DIR}"
        COMMENT "Deploying compiled shaders"
)
