cmake_minimum_required(VERSION 3.22.6)

include(FetchContent)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG ${MLS_SFML_TAG})
FetchContent_MakeAvailable(SFML)

add_subdirectory(basic)
add_subdirectory(file-watch)
