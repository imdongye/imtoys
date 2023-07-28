# 2023-07-09 / imdongye
# cmake -Bbuild
# cmake --build build --config Debug
# x64만 지원, x86하려면 라이브러리 x86빌드 버전 추가해야함
# mac에서 assimp, glfw등은 내장된것을 가져와서 쓰기때문에 플렛폼별로 .cmake를 가져와서 연결한다.

cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

set(sln_name imtoys)
set(exe_name imtoys_exe)
set(main_dir ${CMAKE_SOURCE_DIR}/main) # working dir
file(GLOB_RECURSE main_files CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/main/*.cpp
    ${CMAKE_SOURCE_DIR}/main/*.h 
    ${CMAKE_SOURCE_DIR}/main/*.hpp
    ${CMAKE_SOURCE_DIR}/main/*.vs
    ${CMAKE_SOURCE_DIR}/main/*.fs
    ${CMAKE_SOURCE_DIR}/main/*.vert
    ${CMAKE_SOURCE_DIR}/main/*.frag
)
set(main_include_dirs "limbrary-0.1") # edit here

project(
    ${sln_name}
    VERSION 0.1
    DESCRIPTION "cmake-opengl-template"
    LANGUAGES C CXX
)
set(CMAKE_VERBOSE_MAKEFILE true)

# limbrary
file(GLOB_RECURSE lim_headers CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/limbrary-0.1/*.h 
    ${CMAKE_SOURCE_DIR}/limbrary-0.1/*.hpp 
    ${CMAKE_SOURCE_DIR}/limbrary-0.1/*.inl
)
add_custom_target(limbrary ALL
    SOURCES ${lim_headers}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/limbrary-0.1"
    COMMENT "[LIM] add limbrary"
)
source_group(TREE "${CMAKE_SOURCE_DIR}/limbrary-0.1" FILES ${lim_headers})


#리소스복사 Todo: packing
# file(GLOB_RECURSE lim_resource 
#     ${CMAKE_SOURCE_DIR}/resource/*.png
# )
# add_custom_target(copy_resource ALL
#     SOURCES ${lim_resource}
#     WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/resource"
#     COMMENT "[lim] copying resources into binary directory"
# )
# source_group(TREE "${CMAKE_SOURCE_DIR}/resource" FILES ${lim_resource})

# main proj
if(WIN32) 
    message("[LIM] detect windows")
    include(for_win.cmake)
endif(WIN32)
if(APPLE)
    message("[LIM] detect mac")
    include(for_mac.cmake)
endif(APPLE)

# 모든 경고 표시 및 컴파일에러처리
#target_compile_options(${proj_name} PUBLIC -Wall -Werror)

target_compile_definitions(${exe_name} PUBLIC
    WINDOW_NAME="opengl template window"
    WINDOW_WIDTH=600
    WINDOW_HEIGHT=400
)