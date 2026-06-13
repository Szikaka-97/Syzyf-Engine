file(GLOB_RECURSE GAME_SOURCE_FILES *.cpp *.c)
file(GLOB_RECURSE GAME_HEADER_FILES *.h *.hpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GAME_SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${GAME_HEADER_FILES})

add_executable(Bimberman ${GAME_SOURCE_FILES} ${GAME_HEADER_FILES})

target_link_options(Bimberman PRIVATE "-Wl,--allow-multiple-definition")
target_compile_definitions(Bimberman PRIVATE GLM_ENABLE_EXPERIMENTAL)
target_link_libraries(Bimberman PRIVATE Syzyf)

target_include_directories(Bimberman PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb_image_resize2
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(Bimberman PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

if (WIN32)
    # add_custom_command(
    #         TARGET Bimberman POST_BUILD
    #         COMMAND ${CMAKE_COMMAND} -E copy
    #         ${CMAKE_CURRENT_BINARY_DIR}/../_deps/joltphysics-build/libJolt_d.dll
    #         ${CMAKE_CURRENT_BINARY_DIR}/libJolt_d.dll
    # ) # Someone fix it for fucks sake

endif()

add_custom_command(TARGET Bimberman POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink
                   ${CMAKE_SOURCE_DIR}/res
                   ${CMAKE_CURRENT_BINARY_DIR}/res)
