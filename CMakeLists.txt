# 2023-07-09 / imdongye
# cmake -Bbuild
# cmake --build build --config Debug
# x64만 지원, x86하려면 라이브러리 x86빌드 버전 추가해야함
# mac에서 assimp, glfw등은 내장된것을 가져와서 쓰기때문에 플렛폼별로 .cmake를 가져와서 연결한다.

cmake_minimum_required(VERSION 3.17 FATAL_ERROR)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

# global variables
set(sln_name imtoys)
set(exe_name imtoys_exe)
set(main_dir ${CMAKE_SOURCE_DIR}/main) # working dir

set(glfw_include_dir)
set(glfw_lib_dir)
set(glfw_lib)
set(assimp_include_dir)
set(assimp_lib_dir)
set(assimp_lib)

project(
    ${sln_name}
    VERSION 0.1
    DESCRIPTION "cmake-opengl-template"
    LANGUAGES C CXX
)
set(CMAKE_VERBOSE_MAKEFILE true)

# CMake에서 빌드된 라이브러리는 .lib나 .a를 생략해도되고 include dir도 자동으로 연결된다.
# glfw3.lib를 imgui 정적 라이브러리와 링킹
# todo: TARGET_OBJECT로 glad와 imgui 오브젝트로 실행파일 생성하기
if(WIN32)
    message("[LIM] detect windows")
    set(glfw_include_dir "vendor/glfw-3.3.8/include")
    set(glfw_lib_dir "${CMAKE_SOURCE_DIR}/vendor/glfw-3.3.8/lib-vc2022")
    set(glfw_lib "glfw3.lib")
    set(assimp_include_dir "vendor/assimp-5.2.5/include")
    set(assimp_lib_dir "${CMAKE_SOURCE_DIR}/vendor/assimp-5.2.5/lib/Debug/")
    set(assimp_lib "assimp-vc143-mt$<$<CONFIG:Debug>:d>.lib")
endif(WIN32)

# mac은 glfw이름을 "glfw3"로 find_package 하고 사용할땐 "glfw"로 사용한다
# Opengl:GL은 선택적으로
# xcode gl deprecated 경고 방지 플래그 #define GL_SILENCE_DEPRECATION
# Xcode 14.0.x	macOS Monterey 12.5	macOS 12.3
# Xcode 13.4	macOS Monterey 12	macOS 12.3
# Xcode:17, Xcode12:20, Xcode14:23
# opengl function pointers (replaced by glad)
if(APPLE)
    message("[LIM] detect mac")
    find_package(OpenGL REQUIRED)
    find_package(glfw3 3.3 REQUIRED)
    set(glfw_lib "glfw")
    find_package(ASSIMP REQUIRED)
    set(assimp_lib ${ASSIMP_LIBRARIES})
endif(APPLE)

# glad
set(lib_name "glad")
if(WIN32)
    set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/glad-gl4.6-core")
endif(WIN32)
if(APPLE)
    set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/glad-gl4.1-core")
endif(APPLE)
file(GLOB_RECURSE headers "${lib_dir}/*.h" "${lib_dir}/*.hpp")
file(GLOB_RECURSE sources "${lib_dir}/*.c" "${lib_dir}/*.cpp")
list(LENGTH headers nr_headers)
list(LENGTH sources nr_sources)
message("[LIM] load ${lib_name} from ${lib_dir} with ${nr_headers} headers and ${nr_sources} sources...")
add_library(${lib_name} STATIC ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}/include")
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})

# imgui
set(lib_name "imgui")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/imgui-docking-1.89.4")
file(GLOB_RECURSE headers "${lib_dir}/*.h" "${lib_dir}/*.hpp")
file(GLOB_RECURSE sources "${lib_dir}/*.c" "${lib_dir}/*.cpp")
file(GLOB_RECURSE ignore_files "${lib_dir}/ignore/*")
list(REMOVE_ITEM sources "${lib_dir}/misc/freetype/imgui_freetype.cpp")
list(REMOVE_ITEM headers "${lib_dir}/misc/freetype/imgui_freetype.h")
list(LENGTH headers nr_headers)
list(LENGTH sources nr_sources)
message("[LIM] load ${lib_name} from ${lib_dir} with ${nr_headers} headers and ${nr_sources} sources...")
add_library(${lib_name} STATIC ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}" ${glfw_include_dir})
target_link_directories(${lib_name} PUBLIC ${glfw_lib_dir})
target_link_libraries(${lib_name} PUBLIC ${glfw_lib})
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})


### nanovg ###
set(lib_name "nanovg")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/others/nanovg")
file(GLOB_RECURSE headers "${lib_dir}/*.h" "${lib_dir}/*.hpp")
file(GLOB_RECURSE sources "${lib_dir}/*.c" "${lib_dir}/*.cpp")
list(LENGTH headers nr_headers)
list(LENGTH sources nr_sources)
message("[LIM] load ${lib_name} from ${lib_dir} with ${nr_headers} headers and ${nr_sources} sources...")
add_library(${lib_name} STATIC ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}/include")
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})



### limbrary ###
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
set(main_include_dirs )



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



### main proj ###
file(GLOB_RECURSE main_files CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/main/*.cpp
    ${CMAKE_SOURCE_DIR}/main/*.h 
    ${CMAKE_SOURCE_DIR}/main/*.hpp
    ${CMAKE_SOURCE_DIR}/main/*.vs
    ${CMAKE_SOURCE_DIR}/main/*.fs
    ${CMAKE_SOURCE_DIR}/main/*.vert
    ${CMAKE_SOURCE_DIR}/main/*.frag
)
set(main_include_dirs
    "vendor/glm-0.9.9.8"
    "vendor/eigen-3.4.0"
    "vendor/others"
    "limbrary-0.1"
    ${assimp_include_dir}
)
set(main_lib_dirs
    ${assimp_lib_dir}
)
set(main_libs
    glad
    imgui
    nanovg
    ${assimp_lib}
)

add_executable(${exe_name} ${main_files})
target_include_directories(${exe_name} PUBLIC ${main_include_dirs})
target_link_directories(${lib_name} PUBLIC ${main_lib_dirs})
target_link_libraries(${exe_name} PUBLIC ${main_libs})
source_group(TREE ${main_dir} FILES ${main_files})
add_dependencies(${exe_name} imgui glad nanovg)


### 시작프로젝트 설정 ###
if(CMAKE_GENERATOR MATCHES "Visual Studio*")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${working_dir})
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${exe_name})
    set_property(TARGET ${exe_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${main_dir})
endif()
if(CMAKE_GENERATOR MATCHES "Xcode")
    set_target_properties(${exe_name} PROPERTIES 
                      XCODE_GENERATE_SCHEME TRUE
                      XCODE_SCHEME_WORKING_DIRECTORY "${main_dir}")
    # 기본 scheme(시작프로젝트) 설정하고 싶은데 지원안하는듯
endif()

# 모든 경고 표시 및 컴파일에러처리
#target_compile_options(${proj_name} PUBLIC -Wall -Werror)

### define 변수 설정 ###
target_compile_definitions(${exe_name} PUBLIC
    WINDOW_NAME="opengl template window"
    WINDOW_WIDTH=600
    WINDOW_HEIGHT=400
)