# Prevent this file from being included more than once
if(GOOGLETEST_INCLUDED)
    return()
endif()
set(GOOGLETEST_INCLUDED TRUE)

include(FetchContent)
FetchContent_Declare(
    googletest 
    GIT_REPOSITORY https://github.com/google/googletest.git
    #update release version frequently
    GIT_TAG release-1.12.1
) 
FetchContent_GetProperties(googletest)
if(NOT googletest_POPULATED) 
    FetchContent_Populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BUILD_DIR})
endif()

enable_testing()


file(GLOB_RECURSE TEST_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*/*.cc" #include everything
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/*.cc"
    # header don't need to be included but this might be necessary for some IDEs
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

add_executable(
    AllTests
    ${TEST_SRC}
)

target_include_directories(
    AllTests
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/libs
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(
    AllTests
    gtest 
    gmock
    gtest_main
)

include(GoogleTest)
