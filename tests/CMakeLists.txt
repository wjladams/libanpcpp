include(FetchContent)

if(NOT TARGET GTest::gtest_main)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
  )

  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

add_executable(anpcpp_tests
  test_matrix.cpp
  test_vector.cpp
  test_eigen.cpp
  test_inconsistency.cpp
  test_pairwise.cpp
  test_ratings.cpp
  test_limit_matrix.cpp
  test_network.cpp
  test_subnet.cpp
  test_synthesis.cpp
  test_json_io.cpp
  test_rowsens.cpp
)

target_link_libraries(anpcpp_tests
  PRIVATE
    anpcpp::anpcpp
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(anpcpp_tests)
