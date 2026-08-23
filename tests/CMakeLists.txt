include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(
  slipstream_tests
  test_encode.cpp
  test_parser.cpp
  test_md.cpp
)

target_link_libraries(slipstream_tests PRIVATE slipstream GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(
  slipstream_tests
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)