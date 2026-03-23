# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-src")
  file(MAKE_DIRECTORY "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-build"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-subbuild/joltphysics-populate-prefix"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-subbuild/joltphysics-populate-prefix/tmp"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-subbuild/joltphysics-populate-prefix/src/joltphysics-populate-stamp"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-subbuild/joltphysics-populate-prefix/src"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-subbuild/joltphysics-populate-prefix/src/joltphysics-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-subbuild/joltphysics-populate-prefix/src/joltphysics-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/joltphysics-subbuild/joltphysics-populate-prefix/src/joltphysics-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
