# CPM - package manager
include(CPM)

# OpenGL
find_package(OpenGL REQUIRED)

# glad
set(GLAD_DIR ${CMAKE_CURRENT_LIST_DIR}/glad)
set(glad_SOURCE_DIR ${GLAD_DIR}/include CACHE INTERNAL "")
add_library(glad STATIC ${GLAD_DIR}/src/glad.c)
target_include_directories(glad PRIVATE ${GLAD_DIR}/include)

# stb_image
set(STB_IMAGE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb_image)
set(stb_image_SOURCE_DIR ${STB_IMAGE_DIR} CACHE INTERNAL "")
add_library(stb_image STATIC ${STB_IMAGE_DIR}/stb_image.cpp)
target_include_directories(stb_image PRIVATE ${STB_IMAGE_DIR})

# fastnoiselite
set(FASTNOISELITE_DIR ${CMAKE_CURRENT_LIST_DIR}/fastnoiselite CACHE INTERNAL "")

# other
CPMAddPackage("gh:assimp/assimp@6.0.4")
CPMAddPackage("gh:g-truc/glm#1.0.3")
CPMAddPackage("gh:ocornut/imgui@1.92.5")
CPMAddPackage("gh:gabime/spdlog@1.17.0")
CPMAddPackage(
    NAME fastgltf
    GITHUB_REPOSITORY spnda/fastgltf
    GIT_TAG v0.9.0
    OPTIONS "FASTGLTF_COMPILE_AS_CPP20"
)
CPMAddPackage(
    NAME SDL3
    GITHUB_REPOSITORY libsdl-org/SDL
    GIT_TAG release-3.4.2
    OPTIONS "SDL_X11_XSCRNSAVER OFF"
)
CPMAddPackage(
    NAME JoltPhysics
    GITHUB_REPOSITORY jrouwe/JoltPhysics
    GIT_TAG v5.5.0
    SOURCE_SUBDIR "Build"
    OPTIONS
        "TARGET_UNIT_TESTS OFF"
        "TARGET_SAMPLES OFF"
        "TARGET_PERFORMANCE_TEST OFF"
        "TARGET_VIEWER OFF"
        "TARGET_HELLO_WORLD OFF"
        "DEBUG_RENDERER ON"
        "CPP_RTTI_ENABLED ON"
        "DEBUG_RENDERER_IN_DISTRIBUTION ON"
	    "BUILD_SHARED_LIBS ON"
)
if (NOT MSVC)
target_compile_options(Jolt PRIVATE -Wno-array-bounds)
endif ()

set(imgui_SOURCE_DIR ${imgui_SOURCE_DIR} CACHE INTERNAL "")
add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp
					     ${imgui_SOURCE_DIR}/imgui_demo.cpp
					     ${imgui_SOURCE_DIR}/imgui_draw.cpp
					     ${imgui_SOURCE_DIR}/imgui_tables.cpp
					     ${imgui_SOURCE_DIR}/imgui_widgets.cpp)

set_target_properties(glad
                      stb_image
                      assimp
                      SDL3-static
                      glm
                      imgui
                      spdlog
                      fastgltf
                      imgui
                      Jolt
                      spdlog PROPERTIES FOLDER "thirdparty")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "thirdparty")
endif()
