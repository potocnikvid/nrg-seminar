cmake_minimum_required(VERSION 3.16)
if(POLICY CMP0177)
  cmake_policy(SET CMP0177 NEW)
endif()
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "")
project(nrg-seminar LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG        1.0.1
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glm)

set(GLAD_PROFILE "core"  CACHE STRING "" FORCE)
set(GLAD_API     "gl=3.3" CACHE STRING "" FORCE)
FetchContent_Declare(glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG        v0.1.36
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(glad)

FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.90.1
  GIT_SHALLOW    TRUE
)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

set(STB_DIR "${CMAKE_BINARY_DIR}/_deps/stb")
file(MAKE_DIRECTORY "${STB_DIR}")
if(NOT EXISTS "${STB_DIR}/stb_image.h")
  message(STATUS "Downloading stb_image.h ...")
  file(DOWNLOAD
    "https://raw.githubusercontent.com/nothings/stb/master/stb_image.h"
    "${STB_DIR}/stb_image.h"
  )
endif()

add_executable(nrg_seminar
  src/main.cpp
  src/gl_context.cpp
  src/env_map.cpp
  src/irradiance.cpp
  src/prefilter.cpp
  src/brdf_lut.cpp
  src/pbr_renderer.cpp
  src/ui.cpp
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(nrg_seminar PRIVATE
  ${CMAKE_SOURCE_DIR}/src
  ${STB_DIR}
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
  ${glm_SOURCE_DIR}
)

target_link_libraries(nrg_seminar PRIVATE glad glfw)

if(UNIX AND NOT APPLE)
  target_link_libraries(nrg_seminar PRIVATE dl)
endif()

add_custom_command(TARGET nrg_seminar POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
  ${CMAKE_SOURCE_DIR}/shaders $<TARGET_FILE_DIR:nrg_seminar>/shaders
)
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
  add_custom_command(TARGET nrg_seminar POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:nrg_seminar>/assets
  )
endif()
