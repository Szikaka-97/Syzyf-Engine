file(GLOB_RECURSE EDITOR_SOURCE_FILES
     *.c
     *.cpp)

file(GLOB_RECURSE EDITOR_HEADER_FILES
     *.h
     *.hpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EDITOR_SOURCE_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${EDITOR_HEADER_FILES})

add_executable(SyzyfEditor ${EDITOR_SOURCE_FILES} ${EDITOR_HEADER_FILES})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(SyzyfEditor PROPERTIES WIN32_EXECUTABLE TRUE)
endif()

target_link_libraries(SyzyfEditor PRIVATE Syzyf)

target_link_libraries(SyzyfEditor PRIVATE imgui SDL3::SDL3 glad)

target_include_directories(SyzyfEditor PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/stb_image_resize2
)

add_custom_command(TARGET SyzyfEditor POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E create_symlink
                   ${CMAKE_SOURCE_DIR}/res
                   ${CMAKE_CURRENT_BINARY_DIR}/res)

add_custom_command(TARGET SyzyfEditor POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_if_different
                   ${CMAKE_CURRENT_SOURCE_DIR}/default_editor_layout.ini
                   ${CMAKE_CURRENT_BINARY_DIR}/default_editor_layout.ini)
