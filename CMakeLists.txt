cmake_minimum_required(VERSION 2.8)
project(coroserver)

find_package(Boost 1.53.0 COMPONENTS system context thread REQUIRED)
file(GLOB SOURCE_LIST "*.cpp")
add_executable(coroserver ${SOURCE_LIST})
TARGET_LINK_LIBRARIES(coroserver ${Boost_LIBRARIES})


