cmake_minimum_required(VERSION 3.21 FATAL_ERROR)
project(Syzyf VERSION 1.0)

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
if(CCACHE_PATH)
  message(STATUS "Using ccache: ${CCACHE_PATH}")
  set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PATH}")
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PATH}")
endif()

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
