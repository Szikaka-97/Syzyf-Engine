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

# stb_include
set(STB_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/stb_include)
set(stb_include_SOURCE_DIR ${STB_INCLUDE_DIR} CACHE INTERNAL "")
add_library(stb_include STATIC ${STB_INCLUDE_DIR}/stb_include.cpp)
target_include_directories(stb_include PRIVATE ${STB_INCLUDE_DIR})

# other
CPMAddPackage("gh:assimp/assimp@6.0.4")
CPMAddPackage("gh:glfw/glfw#3.4")
CPMAddPackage("gh:g-truc/glm#1.0.3")
CPMAddPackage("gh:ocornut/imgui@1.92.5")
CPMAddPackage("gh:gabime/spdlog@1.17.0")
CPMAddPackage("gh:freetype/freetype#VER-2-14-3")

set(imgui_SOURCE_DIR ${imgui_SOURCE_DIR} CACHE INTERNAL "")
add_library(imgui STATIC ${imgui_SOURCE_DIR}/imgui.cpp
					     ${imgui_SOURCE_DIR}/imgui_demo.cpp
					     ${imgui_SOURCE_DIR}/imgui_draw.cpp
						 ${imgui_SOURCE_DIR}/imgui_tables.cpp
					     ${imgui_SOURCE_DIR}/imgui_widgets.cpp)
						 
set_target_properties(glad
                      stb_image 
                      stb_include
                      assimp 
                      glfw 
                      glm 
                      imgui 
                      spdlog
                      freetype PROPERTIES FOLDER "thirdparty")

if (TARGET zlibstatic)
    set_target_properties(zlibstatic PROPERTIES FOLDER "thirdparty")
endif()
