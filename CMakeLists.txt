#[==[
  
2023-07-09 / imdongye

cmake -B build . -G [XCode, Ninja ...]
cmake --build build --config Debug

실행경로는 main 폴더, assets폴더를 사용했다면 실행파일을 배포할때 assets 폴더도 같이 배포해야한다.
assimp, glfw 의 BUILD_SHARED_LIBS 옵션을 삭제하고 현재 스코프의 BUILD_SHARED_LIBS를 참조하게 한다.

Note:
* target_link_libraries로 연결한 cmake 라이브러리(.lib나 .a를 생략해도 됨)는 target_include_directories 로 헤더 경로를 입력 안해줘도 자동으로 된다.

Todo:
1. add_lib( OBJECT로 정적라이브러리 생상하지 않고 오브젝트로 실행파일 생성하기
2. 다수의 add_exe.. 골라 실행 시스템

]==]


cmake_minimum_required(VERSION 3.24...3.27 FATAL_ERROR)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DLIM_DEBUG")

set(CMAKE_VERBOSE_MAKEFILE true)

project(
    imtoys
    VERSION 0.1
    DESCRIPTION "imtoys"
    LANGUAGES C CXX
)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    set(CMAKE_GENERATOR_PLATFORM x64)
endif()




set(dep_libs)

### glfw ###
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)
set_target_properties(glfw PROPERTIES CONFIG Release)
target_compile_definitions(glfw INTERFACE GLFW_INCLUDE_NONE) # Todo?: glad include 순서 무시하기위함
set_target_properties(glfw PROPERTIES FOLDER vendor)
set(dep_libs ${dep_libs} glfw)




### assimp ###
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
option(ASSIMP_BUILD_ALL_EXPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_OBJ_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_GLTF_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_STL_IMPORTER ON CACHE BOOL "" FORCE)
    # set(ASSIMP_BUILD_BLEND_IMPORTER ON CACHE BOOL "" FORCE)
option(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF)
    set(ASSIMP_BUILD_OBJ_EXPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_EXPORTER ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_STL_EXPORTER ON CACHE BOOL "" FORCE)
    # set(ASSIMP_BUILD_GLTF_EXPORTER ON CACHE BOOL "" FORCE)
add_subdirectory(vendor/assimp)
set_target_properties(assimp PROPERTIES CONFIG Release) # release 빌드
option(BUILD_SHARED_LIBS OFF)
set_target_properties(assimp PROPERTIES FOLDER vendor)
list(APPEND dep_libs assimp)




### glad ### C
# opengl function pointers (replaced by glad)
set(lib_name glad)
if(WIN32)
    set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/glad-4.6-core)
else()
    set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/glad-4.1-core)
endif(WIN32)
file(GLOB_RECURSE lib_files ${lib_dir}/*.*)
add_library(${lib_name} STATIC ${lib_files})
source_group(TREE ${lib_dir} FILES ${lib_files})
target_include_directories(${lib_name} PUBLIC ${lib_dir}/include)
set_target_properties(${lib_name} PROPERTIES FOLDER vendor)
list(APPEND dep_libs ${lib_name})




### stb ### C
set(lib_name stb)
set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/stb)
file(GLOB_RECURSE lib_files ${lib_dir}/*.c ${lib_dir}/*.h)
add_library(${lib_name} STATIC ${lib_files})
source_group(TREE ${lib_dir} FILES ${lib_files})
target_include_directories(${lib_name} PUBLIC ${lib_dir})
set_target_properties(${lib_name} PROPERTIES FOLDER vendor)
list(APPEND dep_libs ${lib_name})




### imgui ###
set(lib_name imgui)
set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/imgui-docking)
file(GLOB_RECURSE lib_files ${lib_dir}/*.h ${lib_dir}/*.cpp)
list(REMOVE_ITEM lib_files ${lib_dir}/example_glfw_opengl3/main.cpp)
list(REMOVE_ITEM lib_files ${lib_dir}/misc/freetype/imgui_freetype.h)
list(REMOVE_ITEM lib_files ${lib_dir}/misc/freetype/imgui_freetype.cpp)
list(REMOVE_ITEM lib_files ${lib_dir}/misc/fonts/binary_to_compressed_c.cpp)
add_library(${lib_name} STATIC ${lib_files})
source_group(TREE ${lib_dir} FILES ${lib_files})
target_include_directories(${lib_name} PUBLIC ${lib_dir})
target_link_libraries(${lib_name} PUBLIC glfw stb)
add_dependencies(${lib_name} glfw stb)
target_compile_definitions(${lib_name}
    PRIVATE
        IMGUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
        IMGUI_STB_TRUETYPE_FILENAME="../stb/stb_truetype.h"
        IMGUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
        IMGUI_STB_RECT_PACK_FILENAME="../stb/stb_rect_pack.h"
)
set_target_properties(${lib_name} PROPERTIES FOLDER vendor)
list(APPEND dep_libs ${lib_name})




set(lib_name imguizmo)
set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/imguizmo)
file(GLOB_RECURSE lib_files ${lib_dir}/*.h ${lib_dir}/*.cpp)
list(REMOVE_ITEM lib_files ${lib_dir}/imguizmo/example.cpp)
add_library(${lib_name} STATIC ${lib_files})
source_group(TREE ${lib_dir} FILES ${lib_files})
target_include_directories(${lib_name} PUBLIC ${lib_dir})
target_link_libraries(${lib_name} PUBLIC imgui)
add_dependencies(${lib_name} imgui)
target_compile_definitions(${lib_name} PRIVATE USE_IMGUI_API)
set_target_properties(${lib_name} PROPERTIES FOLDER vendor)
list(APPEND dep_libs ${lib_name})






### glm (header only) ###
set(lib_name glm)
set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/glm)
file(GLOB_RECURSE lib_files ${lib_dir}/*.*)
add_library(${lib_name} INTERFACE ${lib_files})
source_group(TREE ${lib_dir}/glm FILES ${lib_files})
target_include_directories(${lib_name} INTERFACE ${lib_dir})
target_compile_definitions(${lib_name} 
    INTERFACE
        GLM_FORCE_SILENT_WARNINGS
        GLM_ENABLE_EXPERIMENTAL # for transform and norm
)
set_target_properties(${lib_name} PROPERTIES FOLDER vendor)
list(APPEND dep_libs ${lib_name})




### eigen (header only) ###
set(lib_name eigen)
set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/eigen)
file(GLOB_RECURSE lib_files ${lib_dir}/*.*)
add_library(${lib_name} INTERFACE ${lib_files})
source_group(TREE ${lib_dir}/Eigen FILES ${lib_files})
target_include_directories(${lib_name} INTERFACE ${lib_dir})
set_target_properties(${lib_name} PROPERTIES FOLDER vendor)
list(APPEND dep_libs ${lib_name})




### json (header only) ###
set(lib_name json)
set(lib_dir ${CMAKE_SOURCE_DIR}/vendor/json)
file(GLOB_RECURSE lib_files ${lib_dir}/nlohmann/json.hpp)
add_library(${lib_name} INTERFACE ${lib_files})
source_group(TREE ${lib_dir} FILES ${lib_files})
target_include_directories(${lib_name} INTERFACE ${lib_dir})
set_target_properties(${lib_name} PROPERTIES FOLDER vendor)
list(APPEND dep_libs ${lib_name})




### limbrary ###
set(lib_name limbrary)
set(lib_dir ${CMAKE_SOURCE_DIR}/limbrary)
file(GLOB_RECURSE lib_files CONFIGURE_DEPENDS ${lib_dir}/*.*)
add_library(${lib_name} STATIC ${lib_files})
source_group(TREE ${lib_dir} FILES ${lib_files})
target_include_directories(${lib_name} PUBLIC ${lib_dir}/include)
target_link_libraries(${lib_name} PUBLIC ${dep_libs})
add_dependencies(${lib_name} ${dep_libs})
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(${lib_name}
        PUBLIC
            _SCL_SECURE_NO_WARNINGS  # Calling any one of the potentially unsafe methods in the Standard C++ Library
            _CRT_SECURE_NO_WARNINGS  # Calling any one of the potentially unsafe methods in the CRT Library
    )
    target_compile_options(${lib_name} 
        PUBLIC
            /MD$<$<CONFIG:Debug>:d>
            /utf-8
        PRIVATE
            /W4
            /wd4100 # 참조되지않은 매개변수
            /wd4244 # double to float
            /wd4267 # size_t to uint
            /wd4018 # < : signed, unsinged
            /wd4702 # program.cpp 접근할수없는 코드
            #/WX
            #/wd4819 # 코드페이지 949 경고
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_definitions(${lib_name} 
        PUBLIC
            GL_SILENCE_DEPRECATION
    )
    target_compile_options(${lib_name} 
        PRIVATE
            -Wall
            #-Werror
    )
endif()






### main proj ###
set(exe_name imtoys_exe)
set(main_dir ${CMAKE_SOURCE_DIR}/main)
file(GLOB_RECURSE main_files CONFIGURE_DEPENDS 
    ${main_dir}/*.c ${main_dir}/*.cpp ${main_dir}/*.h ${main_dir}/*.hpp ${main_dir}/*.ipp
)
list(FILTER main_files   EXCLUDE REGEX ${main_dir}/assets/*)
file(GLOB_RECURSE shader_files #CONFIGURE_DEPENDS 
    ${main_dir}/*.glsl ${main_dir}/*.vs ${main_dir}/*.vert ${main_dir}/*.fs ${main_dir}/*.frag
)

# using opengl4.6
if(APPLE)
    list(FILTER main_files   EXCLUDE REGEX ${main_dir}/im_pbd/*)
    list(FILTER main_files   EXCLUDE REGEX ${main_dir}/im_anims/app_cloth_gpu.*)
    list(FILTER main_files   EXCLUDE REGEX ${main_dir}/im_anims/app_particle.*)
endif()
list(FILTER main_files   EXCLUDE REGEX ${main_dir}/ignored/*)
list(FILTER shader_files EXCLUDE REGEX ${main_dir}/ignored/*)
list(FILTER main_files   EXCLUDE REGEX ${main_dir}/exports/*)
list(FILTER shader_files EXCLUDE REGEX ${main_dir}/exports/*)

add_executable(${exe_name} ${main_files} ${shader_files})
source_group(TREE ${main_dir} FILES ${main_files} ${shader_files})
target_include_directories(${exe_name} PRIVATE ${main_dir})
target_link_libraries(${exe_name} PRIVATE limbrary)
add_dependencies(${exe_name} limbrary)
if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(${exe_name} PRIVATE
        /W4
        /bigobj # for Eigen
        #/WX
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(${exe_name} PRIVATE
        -Wall
        #-Werror
    )
endif()
target_compile_definitions(${exe_name} PRIVATE
    WINDOW_NAME="imtoys template"
    WINDOW_WIDTH=600
    WINDOW_HEIGHT=400
)






### 결과 파일 생성 위치 지정 ###
set_target_properties(limbrary ${dep_libs} ${exe_name} PROPERTIES 
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/library      # static, dll load lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/module       # dll of module 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>/executable   # exe
) 


### 솔루션탐색기에서 나머지 그룹 묶기 ###
set_target_properties(update_mappings PROPERTIES FOLDER "vendor")
if(WIN32)
    set_target_properties(UpdateAssimpLibsDebugSymbolsAndDLLs PROPERTIES FOLDER "vendor")
endif()
# set_target_properties(zlibstatic PROPERTIES FOLDER "vendor")




set(working_dir "${CMAKE_SOURCE_DIR}/main")

if(CMAKE_GENERATOR MATCHES "Visual Studio*")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${exe_name})
    # From: https://stackoverflow.com/questions/41864259/how-to-set-working-directory-for-visual-studio-2017-rc-cmake-project
    set_target_properties(${exe_name} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${working_dir})
endif()
if(CMAKE_GENERATOR MATCHES "Xcode")
    # 시작 schema 설정 못찾음
    set_target_properties(${exe_name} PROPERTIES 
        XCODE_GENERATE_SCHEME TRUE
        XCODE_SCHEME_WORKING_DIRECTORY ${working_dir}
    )
endif()



### CPack ###
# From: https://github.com/imdongye/immines/blob/main/CMakeLists.txt
# if(WIN32)
#     set_target_properties(${exe_name} PROPERTIES 
#         LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:WINDOWS")
# endif(WIN32)