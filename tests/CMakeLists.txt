include(FetchContent)
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
    GIT_SHALLOW FALSE  # ensure submodules are checked out
)
# Keep GoogleTest out of `cmake --install` — it's a test-only dependency and would otherwise drag its
# headers and static libs into the delegate install tree.
set(INSTALL_GTEST OFF)
FetchContent_MakeAvailable(googletest)

add_executable(delegate_test
    ${CMAKE_CURRENT_SOURCE_DIR}/delegate_test.cpp
)
delegate_configure_test_target(delegate_test)
target_link_libraries(delegate_test PRIVATE delegate::delegate GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(delegate_test)
