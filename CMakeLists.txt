#
#   가방싸기, 여행준비
#   2023-07-09 / imdongye
#
# cmake -B build . -G [XCode, Ninja ...]
# cmake --build build --config Debug
#
# 실행경로는 main 폴더, assets폴더를 사용했다면 실행파일을 배포할때 assets 폴더도 같이 배포해야한다.
# assimp, glfw 의 BUILD_SHARED_LIBS 옵션을 삭제하고 현재 스코프의 BUILD_SHARED_LIBS를 참조하게 한다.
#
# Note:
# * target_link_libraries로 연결한 cmake 라이브러리(.lib나 .a를 생략해도 됨)는 target_include_directories 로 헤더 경로를 입력 안해줘도 자동으로 된다.
#
# Todo:
# 1. add_lib( OBJECT로 정적라이브러리 생상하지 않고 오브젝트로 실행파일 생성하기
# 2. 다수의 add_exe.. 골라 실행 시스템
#

cmake_minimum_required(VERSION 3.24...3.27 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)

# global variables
set(sln_name imtoys)
set(exe_name imtoys_exe)
set(working_dir ${CMAKE_SOURCE_DIR}/main)


project(
    ${sln_name}
    VERSION 0.1
    DESCRIPTION "imtoys"
    LANGUAGES C CXX
)
set(CMAKE_VERBOSE_MAKEFILE true)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    #add_compile_options(/wd4819) # 코드페이지 949 경고
    add_compile_options(/utf-8)
endif()


### glfw ###
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)
target_compile_definitions(glfw INTERFACE GLFW_INCLUDE_NONE) # Todo?: glad include 순서 무시하기위함

### assimp ###
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_BLEND_IMPORTER ON CACHE BOOL "" FORCE)
option(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_OBJ_EXPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_EXPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_EXPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(vendor/assimp)
option(BUILD_SHARED_LIBS OFF)

### glad ### C
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
add_library(${lib_name} STATIC ${headers} ${sources})
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}/include")

### nanovg ### C
set(lib_name "nanovg")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/nanovg")
file(GLOB_RECURSE headers "${lib_dir}/*.h")
file(GLOB_RECURSE sources "${lib_dir}/*.c")
add_library(${lib_name} STATIC ${headers} ${sources})
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC ${lib_dir})

### stb ### C
set(lib_name "stb")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/stb")
file(GLOB_RECURSE headers "${lib_dir}/*.h")
file(GLOB_RECURSE sources "${lib_dir}/*.c")
add_library(${lib_name} STATIC ${headers} ${sources})
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC ${lib_dir})

### imgui ###
set(lib_name "imgui")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/imgui-docking-1.89.4")
file(GLOB_RECURSE headers "${lib_dir}/*.h" "${lib_dir}/*.hpp")
file(GLOB_RECURSE sources "${lib_dir}/*.c" "${lib_dir}/*.cpp")
list(REMOVE_ITEM sources "${lib_dir}/misc/freetype/imgui_freetype.cpp")
list(REMOVE_ITEM sources "${lib_dir}/examples/example_glfw_opengl3/main.cpp")
list(REMOVE_ITEM headers "${lib_dir}/misc/freetype/imgui_freetype.h")
add_library(${lib_name} STATIC ${headers} ${sources})
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}")
target_link_libraries(${lib_name} PUBLIC glfw stb)
target_compile_definitions(${lib_name}
    PRIVATE
        IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
        IMGUI_STB_TRUETYPE_FILENAME="../stb/stb_truetype.h"
        IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
        IMGUI_STB_RECT_PACK_FILENAME="../stb/stb_rect_pack.h"
)
add_dependencies(${lib_name} glfw stb)

### glm (header only) ###
set(lib_name "glm")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/glm-0.9.9.8")
file(GLOB_RECURSE lib_files CONFIGURE_DEPENDS "${lib_dir}/*.*")
add_library(${lib_name} INTERFACE ${lib_files})
source_group(TREE "${lib_dir}/glm" FILES ${lib_files})
target_include_directories(${lib_name} INTERFACE  ${lib_dir})
target_compile_definitions(${lib_name} INTERFACE GLM_FORCE_SILENT_WARNINGS)

### eigen (header only) ###
set(lib_name "eigen")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/eigen-3.4.0")
file(GLOB_RECURSE lib_files CONFIGURE_DEPENDS "${lib_dir}/*")
add_library(${lib_name} INTERFACE ${lib_files})
source_group(TREE "${lib_dir}/Eigen" FILES ${lib_files})
target_include_directories(${lib_name} INTERFACE  ${lib_dir})

### others (header only) ###
set(lib_name "others")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/others")
file(GLOB_RECURSE lib_files CONFIGURE_DEPENDS "${lib_dir}/*.*")
add_library(${lib_name} INTERFACE ${lib_files})
source_group(TREE ${lib_dir} FILES ${lib_files})
target_include_directories(${lib_name} INTERFACE  ${lib_dir})


### limbrary ###
# 배포할땐 cpp를 삭제하고 배포하기로 하자.
set(lib_name "limbrary")
set(lib_dir "${CMAKE_SOURCE_DIR}/limbrary-0.1")
set(dep_libs
    glad
    glm
    eigen
    others
    imgui
    assimp
    nanovg
)
file(GLOB_RECURSE lib_files CONFIGURE_DEPENDS "${lib_dir}/*.*")
add_library(${lib_name} STATIC ${lib_files})
source_group(TREE "${lib_dir}" FILES ${lib_files})
target_include_directories(${lib_name} PUBLIC ${lib_dir})
target_link_directories(${lib_name} PUBLIC)
target_link_libraries(${lib_name} PUBLIC ${dep_libs})
add_dependencies(${lib_name} ${dep_libs})
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(${lib_name}
        PUBLIC
            _SCL_SECURE_NO_WARNINGS  # Calling any one of the potentially unsafe methods in the Standard C++ Library
            _CRT_SECURE_NO_WARNINGS  # Calling any one of the potentially unsafe methods in the CRT Library
    )
    target_compile_options(${lib_name} 
        PRIVATE
            /W4
            /wd4100 # 참조되지않은 매개변수
            /wd4244 # double to float
            /wd4267 # size_t to uint
            /wd4018 # < : signed, unsinged
            /wd4702 # program.cpp 접근할수없는 코드
            #/WX
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(
        PUBLIC
            GL_SILENCE_DEPRECATION
    )
    target_compile_options(${lib_name} 
        PRIVATE
            -Wall
            #-Werror
    )
endif()

### assets ###
# 배포할땐 직접 복붙하기로 하자.
file(GLOB_RECURSE asset_files CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/main/assets/*.*)
add_custom_target(assets ALL SOURCES ${asset_files})
source_group(TREE "${CMAKE_SOURCE_DIR}/main/assets" FILES ${asset_files})

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
list(FILTER main_files EXCLUDE REGEX "${CMAKE_SOURCE_DIR}/main/assets/*")
add_executable(${exe_name} ${main_files})
source_group(TREE "${CMAKE_SOURCE_DIR}/main" FILES ${main_files})
target_include_directories(${exe_name} PRIVATE "${CMAKE_SOURCE_DIR}/main")
target_link_libraries(${exe_name} PRIVATE limbrary)
add_dependencies(${exe_name} limbrary)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${lib_name} 
        PRIVATE
            /W4
            #/WX
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${lib_name} 
        PRIVATE
            -Wall
            #-Werror
    )
endif()
target_compile_definitions(${exe_name} 
    PRIVATE
        WINDOW_NAME="opengl template window"
        WINDOW_WIDTH=600
        WINDOW_HEIGHT=400
)

### 결과 파일 생성 위치 지정 ###
set_target_properties(limbrary ${dep_libs} ${exe_name} glfw PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/library) # static, dll load lib
set_target_properties(limbrary ${dep_libs} ${exe_name} glfw PROPERTIES LIBRARY_OUTPUT_DIRECTORY  ${CMAKE_BINARY_DIR}/$<CONFIG>/module) # dll of module 
set_target_properties(limbrary ${dep_libs} ${exe_name} glfw PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/executable) # exe


### IDE 설정 ###
set_target_properties(glfw PROPERTIES FOLDER "vendor")
set_target_properties(update_mappings PROPERTIES FOLDER "vendor")
set_target_properties(assimp PROPERTIES FOLDER "vendor")
set_target_properties(glad PROPERTIES FOLDER "vendor")
set_target_properties(imgui PROPERTIES FOLDER "vendor")
set_target_properties(nanovg PROPERTIES FOLDER "vendor")
if(WIN32)
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "vendor")
endif()
set_target_properties(zlibstatic PROPERTIES FOLDER "vendor")
set_target_properties(glm PROPERTIES FOLDER "vendor")
set_target_properties(eigen PROPERTIES FOLDER "vendor")
set_target_properties(others PROPERTIES FOLDER "vendor")


if(CMAKE_GENERATOR MATCHES "Visual Studio*")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${exe_name})
    # From: https://stackoverflow.com/questions/41864259/how-to-set-working-directory-for-visual-studio-2017-rc-cmake-project
    set_target_properties(${exe_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${working_dir}")
endif()
if(CMAKE_GENERATOR MATCHES "Xcode")
    set_target_properties(${exe_name} PROPERTIES 
                    XCODE_GENERATE_SCHEME TRUE
                    XCODE_SCHEME_WORKING_DIRECTORY "${working_dir}")
    # 시작 schema 설정 못찾음
endif()
