cmake_minimum_required(VERSION 3.16)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG master)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(ImGui
    GIT_REPOSITORY https://github.com/thedmd/imgui
    GIT_TAG feature/layout)
FetchContent_MakeAvailable(ImGui)
FetchContent_GetProperties(ImGui SOURCE_DIR IMGUI_DIR)

set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml
    GIT_TAG master)
FetchContent_MakeAvailable(ImGui-SFML)

FetchContent_Declare(imnodes
    GIT_REPOSITORY https://github.com/Nelarius/imnodes
    GIT_TAG v0.5)
FetchContent_MakeAvailable(imnodes)
FetchContent_GetProperties(imnodes SOURCE_DIR IMNODES_DIR)

FetchContent_Declare(imnoed
    GIT_REPOSITORY https://github.com/thedmd/imgui-node-editor
    GIT_TAG v0.9.3)
FetchContent_MakeAvailable(imnoed)
FetchContent_GetProperties(imnoed SOURCE_DIR IMNOED_DIR)

file( GLOB SRCS ./src/*.cpp ./src/*.hpp ./src/**/*.cpp ./src/**/*.hpp)

add_executable(CMakeSFMLProject 
    ${SRCS} 
    "${IMNODES_DIR}/imnodes.cpp" 
    "${IMNOED_DIR}/imgui_canvas.cpp" 
    "${IMNOED_DIR}/crude_json.cpp" 
    "${IMNOED_DIR}/imgui_node_editor.cpp" 
    "${IMNOED_DIR}/imgui_node_editor_api.cpp"
)

target_include_directories(CMakeSFMLProject PRIVATE ./src/)

target_include_directories(CMakeSFMLProject PRIVATE "${IMNODES_DIR}")
target_include_directories(CMakeSFMLProject PRIVATE "${IMNOED_DIR}")

target_link_libraries(CMakeSFMLProject PRIVATE sfml-graphics ImGui-SFML::ImGui-SFML)
target_compile_definitions(CMakeSFMLProject PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_features(CMakeSFMLProject PRIVATE cxx_std_23)

add_custom_command(
    TARGET CMakeSFMLProject
    COMMENT "Copy Font"
    PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${IMGUI_DIR}/misc/fonts/Cousine-Regular.ttf $<TARGET_FILE_DIR:CMakeSFMLProject>
    VERBATIM)
    
set_target_properties(
    CMakeSFMLProject PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:CMakeSFMLProject>)

install(TARGETS CMakeSFMLProject)
