# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-src")
  file(MAKE_DIRECTORY "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-src")
endif()
file(MAKE_DIRECTORY
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-build"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-subbuild/assimp-populate-prefix"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-subbuild/assimp-populate-prefix/tmp"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-subbuild/assimp-populate-prefix/src/assimp-populate-stamp"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-subbuild/assimp-populate-prefix/src"
  "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-subbuild/assimp-populate-prefix/src/assimp-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-subbuild/assimp-populate-prefix/src/assimp-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/wkow1/Documents/6sem/engine/cmake-build-debug/_deps/assimp-subbuild/assimp-populate-prefix/src/assimp-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
