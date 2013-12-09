cmake_minimum_required(VERSION 2.8)
project(coroserver)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBOOST_LOG_DYN_LINK=1 -std=c++11 -ftemplate-depth=256")
if(APPLE)
    set(CMAKE_LINK_FLAGS "${CMAKE_LINK_FLAGS} -stdlib=libc++")
endif(APPLE)

find_package(Boost 1.55.0 COMPONENTS system thread coroutine context log log_setup REQUIRED)

INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
)

file(GLOB SOURCE_LIST "*.cpp" "http-parser/http_parser.c")
add_executable(coroserver ${SOURCE_LIST})
TARGET_LINK_LIBRARIES(coroserver ${Boost_LIBRARIES} pthread)


