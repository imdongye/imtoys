# 2023-07-10 / imdongye
# CMake에서 빌드된 라이브러리는 .lib나 .a를 생략해도되고 include dir도 자동으로 연결된다.
# glfw3.lib를 imgui 정적 라이브러리와 링킹
# todo: TARGET_OBJECT로 glad와 imgui 오브젝트로 실행파일 생성하기


# 시작프로젝트 설정
if(CMAKE_GENERATOR MATCHES "Visual Studio*")
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${working_dir})
    set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${exe_name})
endif()

# glad
set(lib_name "glad")
set(lib_dir "${CMAKE_SOURCE_DIR}/vendor/glad-gl4.6-core")
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
target_include_directories(${lib_name} PUBLIC "${lib_dir}" "vendor/glfw-3.3.8/include")
target_link_directories(${lib_name} PUBLIC "${CMAKE_SOURCE_DIR}/vendor/glfw-3.3.8/lib-vc2022")
target_link_libraries(${lib_name} PUBLIC "glfw3.lib")
source_group(TREE "${lib_dir}" FILES ${headers} ${sources})

# nano vg
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

# main proj
add_executable(${exe_name} ${main_files})
target_include_directories(${exe_name} PUBLIC
    "vendor/glm-0.9.9.8"
    "vendor/eigen-3.4.0"
    "vendor/assimp-5.2.5/include"
    "vendor/others"
    ${main_include_dirs}
)
target_link_directories(${lib_name} PUBLIC "${CMAKE_SOURCE_DIR}/vendor/assimp-5.2.5/lib/Debug/")
target_link_libraries(${exe_name} PUBLIC 
    # GLFW is inside of imgui
    imgui glad nanovg "assimp-vc143-mt$<$<CONFIG:Debug>:d>.lib"
)
source_group(TREE "${CMAKE_SOURCE_DIR}/main" FILES ${main_files})
add_dependencies(${exe_name} imgui glad)
# 시작프로젝트 설정
if(CMAKE_GENERATOR MATCHES "Visual Studio*")
    set_property(TARGET ${exe_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${main_dir})
endif()