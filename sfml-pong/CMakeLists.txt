cmake_minimum_required(VERSION 3.15)
project(SFMLPong VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SFML 3 COMPONENTS Graphics Window System REQUIRED)
find_package(OpenGL REQUIRED)

add_executable(pong src/main.cpp)

# Link against the modern SFML CMake targets
target_link_libraries(pong PRIVATE SFML::Graphics SFML::Window SFML::System)

# Copy assets to the runtime directory after build (optional)
add_custom_command(TARGET pong POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:pong>/assets
)

# Small OpenGL cube demo
add_executable(cube src/cube.cpp)

target_link_libraries(cube PRIVATE SFML::Graphics SFML::Window SFML::System OpenGL::GL)

# Copy assets for cube (no assets required now, kept for parity)
add_custom_command(TARGET cube POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:cube>/assets
)
