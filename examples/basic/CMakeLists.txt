cmake_minimum_required(VERSION 3.22.6)

add_executable(MLS_example_basic ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_link_libraries(MLS_example_basic PRIVATE
                                    SFML::Graphics
                                    MLS::MLS
                                    )

target_compile_features(MLS_example_basic PRIVATE cxx_std_23)

target_compile_definitions(MLS_example_basic PRIVATE MLS_EXAMPLE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/.."
)