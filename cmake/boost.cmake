# Find Boost
find_package(Boost REQUIRED)

# Include Boost's headers
target_include_directories(RadixTree
        PRIVATE
            ${Boost_INCLUDE_DIRS}
)