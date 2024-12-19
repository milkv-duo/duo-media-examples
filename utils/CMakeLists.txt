#编译静态库
file(GLOB SRC_LIST *.cpp)
add_library(utils STATIC ${SRC_LIST})