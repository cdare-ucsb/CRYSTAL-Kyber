cmake_minimum_required(VERSION 3.16)
project(KyberPoly LANGUAGES CXX)


set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)

# Disable gtest's automatic install target
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)



# Add include path for headers
include_directories(include)


# Define the main executable target
add_executable(main src/main.cpp)



enable_testing()

add_executable(poly_tests
    tests/test_poly.cpp
)

target_link_libraries(poly_tests
    gtest_main
)

include(GoogleTest)
gtest_discover_tests(poly_tests)



# Add Doxygen target (optional)
find_package(Doxygen)
if (DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/docs)

    add_custom_target(doc
        COMMAND Doxygen ${DOXYGEN_IN}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen"
        VERBATIM
    )
endif()
