# 设置 CMake 最低版本
cmake_minimum_required(VERSION 3.20)

# 定义项目名称
set(projectname videoRecorder)
project(${projectname} VERSION 1.0 LANGUAGES CXX)


# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_compile_options(-Wno-stringop-overflow) # 添加禁用特定警告的选项

# 根据构建类型设置 CMAKE_CXX_FLAGS
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -g -Wall -D_MIDDLEWARE_V2_ -D__CV181X__ -DCV181X")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -O3 -DNDEBUG -D_MIDDLEWARE_V2_ -D__CV181X__ -DCV181X")
endif()

set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR})


set(MIDDLEWARE_PATH "${DUO_BUILDROOT_SDK}/middleware/v2")
set(3RD_PARTY "${CMAKE_CURRENT_SOURCE_DIR}/../3rd_party")
set(SRC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src")


#添加duo-buildroot-sdk/middleware/v2 头文件
include_directories("${MIDDLEWARE_PATH}/include")
include_directories("${MIDDLEWARE_PATH}/include/isp/cv181x")
include_directories("${MIDDLEWARE_PATH}/sample/common")

#添加3rd_party 头文件
include_directories("${3RD_PARTY}/include")
include_directories("${3RD_PARTY}/include/ffmpeg")

#添加ffmpeg 头文件
include_directories("${FFMPEG_PATH}/include")

# src库头文件
include_directories(${SRC_PATH})

#源文件
file(GLOB_RECURSE SOURCES "${SRC_PATH}/*.cpp" "${SRC_PATH}/*.h")
add_executable(${projectname} main.cpp ${SOURCES})

#添加duo-buildroot-sdk/middleware/v2 库
target_link_directories(${projectname} PUBLIC "${MIDDLEWARE_PATH}/lib")
target_link_libraries(${projectname} PUBLIC cvi_ive ini sns_full sample isp vdec venc awb ae af cvi_bin cvi_bin_isp misc isp_algo sys vpu)
target_link_directories(${projectname} PUBLIC "${MIDDLEWARE_PATH}/lib/3rd")
target_link_libraries(${projectname} PUBLIC ini)

#添加3rd_party第三方库
target_link_directories(${projectname} PUBLIC "${3RD_PARTY}/lib")
target_link_libraries(${projectname} PUBLIC spdlog)
target_link_directories(${projectname} PUBLIC "${3RD_PARTY}/lib/Poco")
target_link_libraries(${projectname} PUBLIC PocoNet PocoFoundation PocoUtil PocoJSON PocoXML)

#添加ffmpeg第三方库
target_link_directories(${projectname} PUBLIC "${3RD_PARTY}/lib/ffmpeg")
target_link_libraries(${projectname} PUBLIC avformat avcodec swresample avutil)

target_link_libraries(${projectname} PUBLIC z)

install(TARGETS ${projectname} RUNTIME DESTINATION out)
