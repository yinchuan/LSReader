cmake_minimum_required(VERSION 3.26)
project(LSReader)

set(CMAKE_CXX_STANDARD 17)

# add third party
include(FetchContent)
FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG v1.14.1
        GIT_SHALLOW TRUE
)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG 5bcb2d78a16edd7110e72ef694d229815aa29542
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(googletest spdlog)

# add include
include_directories(include)

# add subdirectories
add_subdirectory(src)
add_subdirectory(tests)