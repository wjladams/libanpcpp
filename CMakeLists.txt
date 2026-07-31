cmake_minimum_required(VERSION 3.20)

project(anpcpp
  VERSION 0.2.0
  DESCRIPTION "Analytic Network Process computational library"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# When consumed as a subproject (FetchContent / add_subdirectory), default
# tests and examples off so parent apps are not forced to build them.
if(CMAKE_SOURCE_DIR STREQUAL PROJECT_SOURCE_DIR)
  set(ANPCPP_IS_TOP_LEVEL ON)
else()
  set(ANPCPP_IS_TOP_LEVEL OFF)
endif()

option(ANPCPP_BUILD_TESTS "Build unit tests" ${ANPCPP_IS_TOP_LEVEL})
option(ANPCPP_BUILD_EXAMPLES "Build example programs" ${ANPCPP_IS_TOP_LEVEL})
option(ANPCPP_BUILD_DOCS "Build Doxygen API documentation" ${ANPCPP_IS_TOP_LEVEL})
option(ANPCPP_INSTALL "Generate install rules" ${ANPCPP_IS_TOP_LEVEL})

include(FetchContent)
if(NOT TARGET nlohmann_json::nlohmann_json)
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

add_library(anpcpp STATIC
  src/matrix.cpp
  src/eigen.cpp
  src/inconsistency.cpp
  src/pairwise.cpp
  src/ratings.cpp
  src/limit_matrix.cpp
  src/network.cpp
  src/synthesis.cpp
  src/json_io.cpp
  src/rowsens.cpp
)
add_library(anpcpp::anpcpp ALIAS anpcpp)

target_include_directories(anpcpp
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# JSON is an implementation detail of json_io.cpp (header-only dependency).
# BUILD_INTERFACE keeps FetchContent's nlohmann_json out of the install export.
target_link_libraries(anpcpp PRIVATE
  $<BUILD_INTERFACE:nlohmann_json::nlohmann_json>
)

if(MSVC)
  target_compile_options(anpcpp PRIVATE /W4)
else()
  target_compile_options(anpcpp PRIVATE -Wall -Wextra -Wpedantic)
endif()

if(ANPCPP_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(ANPCPP_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(ANPCPP_BUILD_DOCS)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Doxygen.cmake")
endif()

if(ANPCPP_INSTALL)
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  install(TARGETS anpcpp
    EXPORT anpcppTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(DIRECTORY include/anpcpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  install(EXPORT anpcppTargets
    FILE anpcppTargets.cmake
    NAMESPACE anpcpp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/anpcpp
  )

  configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/anpcppConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/anpcppConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/anpcpp
  )

  write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/anpcppConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
  )

  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/anpcppConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/anpcppConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/anpcpp
  )
endif()
