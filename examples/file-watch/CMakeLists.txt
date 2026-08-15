cmake_minimum_required(VERSION 3.22.6)

FetchContent_Declare(watcher
    GIT_REPOSITORY https://github.com/e-dant/watcher
    GIT_TAG 0.14.5
    SOURCE_SUBDIR  "_"
)
FetchContent_MakeAvailable(watcher)

add_executable(MLS_example_watch ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_include_directories(MLS_example_watch PUBLIC "${watcher_SOURCE_DIR}/include")

target_link_libraries(MLS_example_watch PRIVATE
                                    SFML::Graphics
                                    MLS::MLS)

target_compile_features(MLS_example_watch PRIVATE cxx_std_23)

target_compile_definitions(MLS_example_watch PRIVATE MLS_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
)