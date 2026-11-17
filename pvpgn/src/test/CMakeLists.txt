add_executable(bnetsrp3_test bnetsrp3_test.cpp )
target_link_libraries(bnetsrp3_test common)
ADD_TEST(bnetsrp3_test bnetsrp3_test)

add_executable(bigint bigint.cpp )
target_link_libraries(bigint common)
ADD_TEST(bigint bigint)

# Unit tests of the REST API helpers (GoogleTest). Skipped when GoogleTest is
# not installed (Debian: libgtest-dev).
if(WITH_BNETD AND WITH_API)
  find_package(GTest)
  include(GoogleTest)
  if(GTest_FOUND)
    add_executable(api_test api_text_test.cpp api_ladder_test.cpp api_rawcmd_test.cpp)
    target_include_directories(api_test PRIVATE ${UTFCPP_INCLUDE_DIR})
    target_link_libraries(api_test common compat GTest::gtest GTest::gtest_main)
    gtest_discover_tests(api_test)
  else()
    message(STATUS "GoogleTest not found: the REST API unit tests are not built")
  endif()
endif()
