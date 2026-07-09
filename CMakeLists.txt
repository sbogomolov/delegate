cmake_minimum_required(VERSION 3.24)
project(delegate
    VERSION 0.1.1
    DESCRIPTION "A small, zero-allocation, type-safe C++ delegate (non-owning bound callable)"
    HOMEPAGE_URL "https://github.com/sbogomolov/delegate"
    LANGUAGES CXX
)

include(CTest)

# Header-only INTERFACE library. The header needs C++20 (concepts, abbreviated `Dependent::Type`,
# `noexcept(bool)` in function-type partial specializations); consumers on a newer standard are fine.
add_library(delegate INTERFACE)
add_library(delegate::delegate ALIAS delegate)
target_include_directories(delegate INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(delegate INTERFACE cxx_std_20)

# --- Install / export, so consumers can `find_package(delegate)` ------------------------------------
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS delegate EXPORT delegate-targets)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(EXPORT delegate-targets
    FILE delegate-targets.cmake
    NAMESPACE delegate::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/delegate
)
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/delegate-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/delegate-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/delegate
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/delegate-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
    ARCH_INDEPENDENT
)
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/delegate-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/delegate-config-version.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/delegate
)

# --- Tests (only when this is the top-level project) ------------------------------------------------
option(DELEGATE_BUILD_TESTS "Build the delegate test suite" ${PROJECT_IS_TOP_LEVEL})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(_delegate_sanitize_default ON)
else()
    set(_delegate_sanitize_default OFF)
endif()
option(DELEGATE_SANITIZE "Build the tests with AddressSanitizer + UndefinedBehaviorSanitizer"
    ${_delegate_sanitize_default})
unset(_delegate_sanitize_default)

# Applies the project's warning set (and optional sanitizers) to a target that compiles the header —
# the header is where any warning surfaces, so the test target carries the same -Werror gauntlet the
# library is held to. Mirrors reflector's flag set so the two stay warning-clean against each other.
function(delegate_configure_test_target target)
    # Strict ISO mode (no GNU extensions). The standard *level* deliberately stays unpinned here so it
    # comes from CMAKE_CXX_STANDARD (CI sweeps C++20 and C++23) over the library's cxx_std_20 floor.
    set_target_properties(${target} PROPERTIES CXX_EXTENSIONS OFF)
    target_compile_options(${target} PRIVATE
        -Werror
        -Wall
        -Wextra
        -Wpedantic
        -Wnon-virtual-dtor
        -Wold-style-cast
        -Wzero-as-null-pointer-constant
        -Wnull-dereference
        -Wconversion
        -Wsign-conversion
        -Wcast-qual
        -Wundef
        -Wformat=2
        -Wswitch-enum
        -Wsuggest-override
        -Woverloaded-virtual
        -Wextra-semi
        $<$<CXX_COMPILER_ID:GNU>:-Wshadow>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wshadow-all>
        $<$<CXX_COMPILER_ID:GNU>:-Wcast-align=strict>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wcast-align>
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond>
        $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-branches>
        $<$<CXX_COMPILER_ID:GNU>:-Wlogical-op>
        $<$<CXX_COMPILER_ID:GNU>:-Wuseless-cast>
        $<$<CXX_COMPILER_ID:GNU>:-Wformat-signedness>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wcomma>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wconditional-uninitialized>
        $<$<CXX_COMPILER_ID:GNU>:-Wmissing-declarations>
        $<$<CXX_COMPILER_ID:Clang,AppleClang>:-Wmissing-prototypes>
        $<$<CONFIG:Debug>:-ggdb3>
    )
    if(DELEGATE_SANITIZE)
        target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

if(DELEGATE_BUILD_TESTS AND BUILD_TESTING)
    add_subdirectory(tests)
endif()
