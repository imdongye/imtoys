#
#   가방싸기, 여행준비
#   2023-07-09 / imdongye
#
# cmake -B build . -G [XCode, Ninja ...]
# cmake --build build --config Debug
#
# 실행경로는 main 폴더, assets폴더를 사용했다면 실행파일을 배포할때 assets 폴더도 같이 배포해야한다.
#
# Note:
# * target_link_libraries로 연결한 cmake 라이브러리(.lib나 .a를 생략해도 됨)는 target_include_directories 로 헤더 경로를 입력 안해줘도 자동으로 된다.
#
# Todo:
# 1. add_lib( OBJECT로 정적라이브럴 생상하지 않고 오브젝트로 실행파일 생성하기
# 2. BUILD_SHARED로 빌드할때 dll 옮기기
# 3. 다수의 add_exe.. 골라 실행 시스템
# 4. glfw 솔루션탐색기 폴더 어떻게 했는지
#
# 1. 헤더온리라이브러리 INTERFACE
# https://stackoverflow.com/questions/60604249/how-to-make-a-header-only-library-with-cmake
# https://dropfl.github.io/blog/2020/12/15/cmake-interface.html



cmake_minimum_required(VERSION 3.26...3.27 FATAL_ERROR)

option(LIM_USE_GLAD "[only mac] If you turn it off, use the default opengl.framework of the mac." ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

set(CMAKE_CXX_FLAGS_RELEASE "/MT")
set(CMAKE_CXX_FLAGS_DEBUG "/MTd")

# global variables
set(sln_name imtoys)
set(exe_name imtoys_exe)
set(working_dir ${CMAKE_SOURCE_DIR}/main)

project(
    ${sln_name}
    VERSION 0.1
    DESCRIPTION "cmake-opengl-template"
    LANGUAGES C CXX
)
set(CMAKE_VERBOSE_MAKEFILE true)

### glfw ###
add_subdirectory(vendor/glfw-3.3.8)

### assimp ###
add_subdirectory(vendor/assimp-5.2.5)

### glad ###
if( WIN32 OR LIM_USE_GLAD )
    # opengl function pointers (replaced by glad)
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
    source_group(TREE "${lib_dir}" FILES ${headers} ${sources})
    target_include_directories(${lib_name} PUBLIC "${lib_dir}/include")
else()
    find_package(OpenGL REQUIRED)
endif()


### imgui ###
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
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}")
target_link_libraries(${lib_name} PUBLIC glfw)
add_dependencies(${lib_name} glfw)


### nanovg ###
set(lib_name "nanovg")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/others/nanovg")
file(GLOB_RECURSE headers "${lib_dir}/*.h" "${lib_dir}/*.hpp")
file(GLOB_RECURSE sources "${lib_dir}/*.c" "${lib_dir}/*.cpp")
list(LENGTH headers nr_headers)
list(LENGTH sources nr_sources)
message("[LIM] load ${lib_name} from ${lib_dir} with ${nr_headers} headers and ${nr_sources} sources...")
add_library(${lib_name} STATIC ${headers} ${sources})
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}/include")


### assets ###
file(GLOB_RECURSE asset_files CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/assets/*.*
)
add_custom_target(assets ALL
    SOURCES ${asset_files}
)
source_group(TREE "${CMAKE_SOURCE_DIR}/main/assets" FILES ${asset_files})

# 배포할땐 직접 복붙하기로 하자.
# add_custom_command(TARGET ${exe_name} POST_BUILD
#     COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different  
#     ${CMAKE_SOURCE_DIR}/assets
#     ${CMAKE_CURRENT_BINARY_DIR}/$<CONFIG>/executable/assets
#     COMMENT "[LIM] copying to executable folder"
# )


### limbrary ###
file(GLOB_RECURSE limb_files CONFIGURE_DEPENDS 
    ${CMAKE_SOURCE_DIR}/limbrary-0.1/*.h 
    ${CMAKE_SOURCE_DIR}/limbrary-0.1/*.hpp 
    ${CMAKE_SOURCE_DIR}/limbrary-0.1/*.inl
)
set(limb_include_dirs
    "vendor/glm-0.9.9.8"
    "vendor/eigen-3.4.0"
    "vendor/others"
    "limbrary-0.1"
)
set(limb_lib_dirs
    # nothing
)
set(limb_libs
    imgui
    assimp::assimp
)
if(WIN32 OR LIM_USE_GLAD)
    list(APPEND limb_libs glad)
else()
    list(APPEND limb_libs ${OPENGL_gl_LIBRARY})
endif()

#add_custom_target(limbrary SOURCES ${limb_files})
add_library(limbrary INTERFACE)
target_include_directories(limbrary INTERFACE  ${limb_include_dirs})
target_link_directories(limbrary INTERFACE  ${limb_lib_dirs})
target_link_libraries(limbrary INTERFACE  ${limb_libs})
target_sources(limbrary INTERFACE ${limb_files})
source_group(TREE "${CMAKE_SOURCE_DIR}/limbrary-0.1" FILES ${limb_files})
add_dependencies(limbrary ${limb_libs})


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
add_executable(${exe_name} ${main_files})
source_group(TREE "${CMAKE_SOURCE_DIR}/main" FILES ${main_files})
target_link_libraries(${exe_name} PUBLIC limbrary)
add_dependencies(${exe_name} limbrary)


### 결과 파일 생성 위치 지정 ###
set_target_properties(imgui glad nanovg PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/library)
set_target_properties(imgui glad nanovg PROPERTIES LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/$<CONFIG>/library)
set_target_properties(${exe_name} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/executable)


### IDE 설정 ###
if(WIN32)
    message("[LIM] detect windows")
    if(CMAKE_GENERATOR MATCHES "Visual Studio*")
        set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${exe_name})
        set_property(TARGET ${exe_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${workding_dir})
    endif()
endif(WIN32)

if(APPLE)
    message("[LIM] detect mac")
    if(CMAKE_GENERATOR MATCHES "Xcode")
        set_target_properties(${exe_name} PROPERTIES 
                        XCODE_GENERATE_SCHEME TRUE
                        XCODE_SCHEME_WORKING_DIRECTORY "${workding_dir}")
        # 시작 schema 설정 못찾음
    endif()
endif(APPLE)


# 모든 경고 표시 및 컴파일에러처리
#target_compile_options(${proj_name} PUBLIC -Wall -Werror)
# if(MSVC)
#   # Force to always compile with W4
#   if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
#     string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#   else()
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
#   endif()
# elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
#   # Update if necessary
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-long-long -pedantic")
# endif()

### define 변수 설정 ###
target_compile_definitions(${exe_name} PUBLIC
    WINDOW_NAME="opengl template window"
    WINDOW_WIDTH=600
    WINDOW_HEIGHT=400
)