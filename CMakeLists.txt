
cmake_minimum_required(VERSION 3.16)
project(radix_tree CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")



# let ccmake and cmake-gui offer the default build type options
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel")

# set Release as the default build type if it is not yet set.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING
            "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# collect all cpp files
file(GLOB_RECURSE MY_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    # header don't need to be included but this might be necessary for some IDEs
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
)

# create make target
add_executable(RadixTree ${MY_SRC})

# set cxx standard. You may raise this if you want.
target_compile_features(RadixTree
        PRIVATE
            cxx_std_17
)

target_include_directories(RadixTree
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src
)


# activate all compiler warnings.
# depending on the compiler different flags are used
target_compile_options(RadixTree
        PRIVATE -Wall
)

#include modules 
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

include(doxygen)
include(googletest)
