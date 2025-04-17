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

find_path(SODIUM_INCLUDE_DIR sodium.h)
find_library(SODIUM_LIB sodium)

if(NOT SODIUM_INCLUDE_DIR OR NOT SODIUM_LIB)
    message(FATAL_ERROR "libsodium not found! Make sure it's installed via Homebrew.")
endif()

target_include_directories(main PRIVATE ${SODIUM_INCLUDE_DIR})
target_link_libraries(main PRIVATE ${SODIUM_LIB})

find_package(OpenSSL REQUIRED)
target_link_libraries(main PRIVATE OpenSSL::Crypto)


enable_testing()

add_executable(modular_int_tests
    tests/test_ModularInt.cpp
)
add_executable(modular_poly_tests
    tests/test_ModularPoly.cpp
)
add_executable(key_gen_tests
    tests/test_KeyGen.cpp
)


target_link_libraries(modular_int_tests
    gtest_main
)
target_link_libraries(modular_poly_tests
    gtest_main
)
target_link_libraries(key_gen_tests
    gtest_main
    OpenSSL::Crypto
)



include(GoogleTest)
gtest_discover_tests(modular_int_tests)
gtest_discover_tests(modular_poly_tests)
gtest_discover_tests(key_gen_tests)



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
