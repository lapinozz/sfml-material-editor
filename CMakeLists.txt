cmake_minimum_required(VERSION 3.16)
project(CMakeSFMLProject LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG master)
FetchContent_MakeAvailable(SFML)

set(IMGUI_DIR "${PROJECT_SOURCE_DIR}/deps/imgui")

set(IMGUI_SFML_FIND_SFML OFF)
FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml
    GIT_TAG master)
FetchContent_MakeAvailable(ImGui-SFML)

set(IMNOED_DIR "${PROJECT_SOURCE_DIR}/deps/imgui-node-editor")

file( GLOB_RECURSE SRCS ${PROJECT_SOURCE_DIR}/src/*.cpp ${PROJECT_SOURCE_DIR}/src/*.hpp )

add_executable(CMakeSFMLProject 
    ${SRCS}
    "${IMNOED_DIR}/imgui_canvas.cpp" 
    "${IMNOED_DIR}/crude_json.cpp" 
    "${IMNOED_DIR}/imgui_node_editor.cpp" 
    "${IMNOED_DIR}/imgui_node_editor_api.cpp"
    "${PROJECT_SOURCE_DIR}/deps/ImGuiColorTextEdit/TextEditor.cpp"
)

target_include_directories(CMakeSFMLProject PRIVATE ${PROJECT_SOURCE_DIR}/src/)
target_include_directories(CMakeSFMLProject PRIVATE ${PROJECT_SOURCE_DIR}/deps/)

target_include_directories(CMakeSFMLProject PRIVATE "${IMNOED_DIR}")

target_link_libraries(CMakeSFMLProject PRIVATE sfml-graphics ImGui-SFML::ImGui-SFML)
target_compile_definitions(CMakeSFMLProject PRIVATE IMGUI_DEFINE_MATH_OPERATORS)
target_compile_features(CMakeSFMLProject PRIVATE cxx_std_23)

#if (MSVC AND WIN32 AND NOT MSVC_VERSION VERSION_LESS 142)
#    target_link_options(CMakeSFMLProject PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL>)
#    target_compile_options(CMakeSFMLProject PRIVATE $<$<CONFIG:Debug>:/ZI>)
#endif()

add_custom_command(
    TARGET CMakeSFMLProject
    COMMENT "Copy Font"
    PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy ${IMGUI_DIR}/misc/fonts/Cousine-Regular.ttf $<TARGET_FILE_DIR:CMakeSFMLProject>
    VERBATIM)
    
set_target_properties(
    CMakeSFMLProject PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:CMakeSFMLProject>)

install(TARGETS CMakeSFMLProject)
