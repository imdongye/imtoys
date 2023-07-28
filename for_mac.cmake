# 2023-07-10 / imdongye
# mac은 glfw이름을 "glfw3"로 find_package 하고 사용할땐 "glfw"로 사용한다
# Opengl:GL은 선택적으로
# xcode gl deprecated 경고 방지 플래그 #define GL_SILENCE_DEPRECATION

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set_property(GLOBAL PROPERTY USE_FOLDERS ON)
# Xcode 14.0.x	macOS Monterey 12.5	macOS 12.3
# Xcode 13.4	macOS Monterey 12	macOS 12.3
# Xcode:17, Xcode12:20, Xcode14:23

# opengl function pointers (replaced by glad)
find_package(OpenGL REQUIRED)

# glfw
find_package(glfw3 3.3 REQUIRED)

# assimp
find_package(ASSIMP REQUIRED)

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
list(REMOVE_ITEM headers "${lib_dir}/misc/freetype/imgui_freetype.h")
list(REMOVE_ITEM sources "${lib_dir}/misc/freetype/imgui_freetype.cpp")
list(LENGTH headers nr_headers)
list(LENGTH sources nr_sources)
message("[LIM] load ${lib_name} from ${lib_dir} with ${nr_headers} headers and ${nr_sources} sources...")
add_library(${lib_name} STATIC ${headers} ${sources})
target_include_directories(${lib_name} PUBLIC "${lib_dir}")
target_link_libraries(${lib_name} PUBLIC glfw)
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
    "vendor/others"
    ${main_include_dirs}
)
target_link_libraries(${exe_name} PUBLIC 
    # GLFW is inside of imgui
    imgui glad OpenGL::GL ${ASSIMP_LIBRARIES} nanovg
)
source_group(TREE ${main_dir} FILES ${main_files})
add_dependencies(${exe_name} imgui glad)

if(CMAKE_GENERATOR MATCHES "Xcode")
    set_target_properties(${exe_name} PROPERTIES 
                      XCODE_GENERATE_SCHEME TRUE
                      XCODE_SCHEME_WORKING_DIRECTORY "${main_dir}")
    # 기본 scheme(시작프로젝트) 설정하고 싶은데 지원안하는듯
endif()