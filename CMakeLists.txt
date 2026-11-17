cmake_minimum_required(VERSION 3.31)
project(power-monitor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "Export compile_commands.json" FORCE)

option(USE_DESKTOP "SDL desktop preview (OFF for Cardputer Zero)" ON)
option(BUILD_GUI "Build LVGL application" ON)
include(CTest)

# A custom BSP sysroot is not a compiler-native sysroot, so GCC does not add
# the Debian multiarch include directory on its own. The official CardputerZero
# template adds the same directory explicitly for its target.
if(NOT USE_DESKTOP AND CMAKE_SYSROOT)
  if(CMAKE_LIBRARY_ARCHITECTURE)
    set(_CP0_MULTIARCH "${CMAKE_LIBRARY_ARCHITECTURE}")
  else()
    set(_CP0_MULTIARCH "aarch64-linux-gnu")
  endif()
  set(_CP0_MULTIARCH_INCLUDE "${CMAKE_SYSROOT}/usr/include/${_CP0_MULTIARCH}")
  if(EXISTS "${_CP0_MULTIARCH_INCLUDE}/bits/libc-header-start.h")
    include_directories(SYSTEM "${_CP0_MULTIARCH_INCLUDE}")
  endif()
endif()

add_library(power_core src/model.cpp src/ascii_view.cpp src/collector.cpp)
target_include_directories(power_core PUBLIC src)
target_compile_options(power_core PRIVATE -Wall -Wextra -Wpedantic)
if(BUILD_TESTING)
  add_executable(power-tests tests/core_test.cpp)
  target_link_libraries(power-tests PRIVATE power_core)
  add_test(NAME power-core COMMAND power-tests)
  add_executable(power-collector-tests tests/collector_test.cpp)
  target_link_libraries(power-collector-tests PRIVATE power_core)
  add_test(NAME collector-lifecycle COMMAND power-collector-tests)
  set_tests_properties(collector-lifecycle PROPERTIES TIMEOUT 15)
endif()

if(BUILD_GUI)
  if(NOT USE_DESKTOP AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
    message(FATAL_ERROR "Device build requires Linux or the CardputerZero BSP toolchain")
  endif()

  include(FetchContent)
  set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
  set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/src/lv_conf.h" CACHE PATH "" FORCE)

  FetchContent_Declare(
    lvgl
    GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG        v9.5.0
    GIT_SHALLOW    TRUE
  )
  FetchContent_GetProperties(lvgl)
  if(NOT lvgl_POPULATED)
    if(POLICY CMP0169)
      cmake_policy(PUSH)
      cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Populate(lvgl)
    if(POLICY CMP0169)
      cmake_policy(POP)
    endif()
  endif()
  add_subdirectory("${lvgl_SOURCE_DIR}" "${lvgl_BINARY_DIR}" EXCLUDE_FROM_ALL)

  target_compile_definitions(lvgl PUBLIC USE_DESKTOP=$<BOOL:${USE_DESKTOP}>)
  if(USE_DESKTOP)
    find_package(SDL2 CONFIG REQUIRED)
    target_link_libraries(lvgl PUBLIC SDL2::SDL2)
  endif()

  add_executable(power-monitor src/main.cpp)
  target_link_libraries(power-monitor PRIVATE power_core lvgl)
  target_compile_options(power-monitor PRIVATE -Wall -Wextra -Wpedantic)

  if(BUILD_TESTING AND USE_DESKTOP)
    add_executable(power-interaction-tests tests/interaction_test.cpp)
    target_link_libraries(power-interaction-tests PRIVATE power_core lvgl)
    target_compile_options(power-interaction-tests PRIVATE -UNDEBUG)
    add_test(NAME interaction COMMAND power-interaction-tests)
    add_executable(power-lifecycle-tests tests/lifecycle_test.cpp)
    target_link_libraries(power-lifecycle-tests PRIVATE power_core lvgl)
    add_test(NAME escape-lifecycle COMMAND power-lifecycle-tests)
    add_test(NAME window-close-lifecycle COMMAND power-lifecycle-tests close)
    set_tests_properties(escape-lifecycle window-close-lifecycle PROPERTIES ENVIRONMENT "SDL_VIDEODRIVER=dummy" TIMEOUT 8)
  endif()

  if(TARGET power-monitor)
    include(cmake/cm0-package.cmake)
  endif()
endif()
