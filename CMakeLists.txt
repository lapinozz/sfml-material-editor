cmake_minimum_required(VERSION 3.22.6)
project(MLS LANGUAGES CXX)

include(GenerateExportHeader)

# define a macro that helps defining an option
macro(mls_set_option var default type docstring)
    if(NOT DEFINED ${var})
        set(${var} ${default})
    endif()
    set(${var} ${${var}} CACHE ${type} ${docstring} FORCE)
endmacro()

option(MLS_FIND_SFML "Use find_package to find SFML" OFF)

if(MLS_FIND_SFML)
  if(NOT BUILD_SHARED_LIBS)
    set(SFML_STATIC_LIBRARIES ON)
  endif()
  find_package(SFML 3 REQUIRED COMPONENTS Graphics)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(MLS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")

file( GLOB_RECURSE MLS_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file( GLOB_RECURSE MLS_INCLUDES "${MLS_INCLUDE_DIR}/*.hpp" )

add_library(MLS ${MLS_SRCS} ${MLS_INCLUDES})
add_library(MLS::MLS ALIAS MLS)

target_compile_features(MLS PRIVATE cxx_std_20)
set_property(TARGET MLS PROPERTY CXX_STANDARD 20)

target_link_libraries(MLS PUBLIC SFML::Graphics)

target_include_directories(MLS PUBLIC
    $<TARGET_PROPERTY:MLS,BINARY_DIR>
    $<TARGET_PROPERTY:MLS,SOURCE_DIR>/include
)

generate_export_header(MLS)

if(BUILD_SHARED_LIBS)
    set_target_properties(MLS PROPERTIES DEBUG_POSTFIX "_d")
endif()

install(TARGETS MLS EXPORT MLSTargets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    PUBLIC_HEADER DESTINATION include
)

install(EXPORT MLSTargets
    FILE MLSConfig.cmake
    DESTINATION share/cmake/MLS
)

install(TARGETS MLS
  EXPORT MLS
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT MLS
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MLS
  NAMESPACE MLS::
)

install(FILES ${PROJECT_SOURCE_DIR}/cmake/MLSConfig.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MLS
)

option(MLS_BUILD_EDITOR "Build the MLS editor" ON)
if(MLS_BUILD_EDITOR)
  add_subdirectory(editor)
endif()

mls_set_option(CLANG_TIDY_EXECUTABLE clang-tidy STRING "Override clang-tidy executable, requires minimum version 14")
add_custom_target(tidy
    COMMAND ${CMAKE_COMMAND} -DCLANG_TIDY_EXECUTABLE=${CLANG_TIDY_EXECUTABLE} -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR} -P ./cmake/Tidy.cmake
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} VERBATIM)