cmake_minimum_required(VERSION 3.30.5 FATAL_ERROR)
project(Syzyf VERSION 1.0)

cmake_policy(SET CMP0171 NEW)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

IF (WIN32)
  message(NOTICE "\n")
  message(NOTICE "********************** IMPORTANT NOTICE ****************************\n")
  message(NOTICE "Make sure you have enabled Developer Mode in your system settings!")
  message(NOTICE "To enable Developer Mode go to Settings -> Update & Security -> For developers -> Developer Mode ON")
  message(NOTICE "\n********************************************************************")
  message(NOTICE "\n")
ENDIF()

find_program(CCACHE_PATH ccache HINTS ENV PATH)
if (CCACHE_PATH)
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PATH}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PATH}")
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

include(global_settings)

# ---- Dependencies ----
add_subdirectory(thirdparty)

if (MSVC)
add_link_options(/NODEFAULTLIB:LIBCMTD)
endif()

set(CMAKE_CXX_STANDARD 23)

IF (MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /vmg /vmv")
ELSEIF (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
ENDIF ()

# ---- Main project's files ----
add_subdirectory(src)
target_include_directories(${PROJECT_NAME} PRIVATE src/include)

# ---- Tracy -----
option(ENABLE_TRACY "Enable Tracy porfiler" OFF)
if (ENABLE_TRACY)
    message(STATUS "Tracy profiler ENABLED")
endif()

# ---- Editor ----
option(BUILD_EDITOR "Build the Syzyf Editor" ON)

if(BUILD_EDITOR)
    message(STATUS "Syzyf Editor build ENABLED")
    add_subdirectory(editor)
endif()

# ---- Game ----
option(BUILD_GAME "Build Game" ON)

if(BUILD_GAME)
    message(STATUS "Game build ENABLED")
    add_subdirectory(game)
endif()

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/../_deps/openal-src/include
        ${CMAKE_CURRENT_BINARY_DIR}/../_deps/openal-src/include/AL
)
