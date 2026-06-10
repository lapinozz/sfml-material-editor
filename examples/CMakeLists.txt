cmake_minimum_required(VERSION 3.22.6)

include(FetchContent)

FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG ${MLS_SFML_TAG})
FetchContent_MakeAvailable(SFML)


add_executable(MLS_example ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(MLS_example PRIVATE
                                    SFML::Graphics
                                    MLS::MLS
                                    )

target_compile_features(MLS_example PRIVATE cxx_std_23)

target_compile_definitions(MLS_example PRIVATE MLS_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)