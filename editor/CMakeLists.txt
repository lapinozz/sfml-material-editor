cmake_minimum_required(VERSION 3.22.6)
project(MLSE LANGUAGES CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

include(FetchContent)

#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

FetchContent_Declare(nativefiledialog
    GIT_REPOSITORY https://github.com/btzy/nativefiledialog-extended
    GIT_TAG v1.2.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(nativefiledialog)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 016bea9491ccafc3529019fe1d403885a8b3a6ae)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(IMGUI
    GIT_REPOSITORY https://github.com/lapinozz/imgui.git
    GIT_TAG origin/MyLittleShader)
FetchContent_MakeAvailable(IMGUI)

set(IMGUI_DIR "${imgui_SOURCE_DIR}")

set(IMGUI_SFML_FIND_SFML OFF)
set(IMGUI_SFML_FIND_IMGUI OFF)
FetchContent_Declare(ImGui-SFML
    GIT_REPOSITORY https://github.com/SFML/imgui-sfml
    GIT_TAG 5e8f8fb857c8f3cff1096bf3e4402c64e3bcf6af)
FetchContent_MakeAvailable(ImGui-SFML)

FetchContent_Declare(nvdialog
    GIT_REPOSITORY https://github.com/tseli0s/nvdialog
    GIT_TAG 1fd880e5561a8782260397451a9538c97c620955
)
FetchContent_MakeAvailable(nvdialog)

FetchContent_Declare(PlatformFolders
    GIT_REPOSITORY https://github.com/sago007/PlatformFolders
    GIT_TAG 4.3.0
)
FetchContent_MakeAvailable(PlatformFolders)

set(IMNOED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/imgui-node-editor")

file( GLOB_RECURSE SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp )

add_executable(MLSE 
    ${SRCS}
    "${IMNOED_DIR}/imgui_canvas.cpp" 
    "${IMNOED_DIR}/crude_json.cpp" 
    "${IMNOED_DIR}/imgui_node_editor.cpp" 
    "${IMNOED_DIR}/imgui_node_editor_api.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/ImGuiColorTextEdit/TextEditor.cpp"
)

target_include_directories(MLSE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(MLSE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/deps/)

target_include_directories(MLSE PRIVATE "${IMNOED_DIR}")

target_link_libraries(MLSE PRIVATE  ImGui-SFML::ImGui-SFML 
                                                nfd::nfd 
                                                nvdialog 
                                                sago::platform_folders
                                                MLS::MLS)

target_compile_definitions(MLSE PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
target_compile_features(MLSE PRIVATE cxx_std_23)

#if (MSVC AND WIN32 AND NOT MSVC_VERSION VERSION_LESS 142)
#    target_link_options(MLSE PRIVATE $<$<CONFIG:Debug>:/INCREMENTAL>)
#    target_compile_options(MLSE PRIVATE $<$<CONFIG:Debug>:/ZI>)
#endif()
    
set_target_properties(
    MLSE PROPERTIES
    VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:MLSE>)

install(TARGETS MLSE)
