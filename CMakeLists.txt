add_definitions(-DHAVE_SYS_TIME_H)
set(CMAKE_CXX_FLAGS "-O2 -g")
add_executable(regex-test regex-test.cpp)
