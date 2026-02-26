cmake_minimum_required(VERSION 3.20)
project(PrimeStage VERSION 0.1.0 LANGUAGES CXX)

option(PRIMESTAGE_FETCH_PRIMEFRAME "Fetch PrimeFrame dependency" ON)
set(PRIMEFRAME_GIT_REPOSITORY "git@github.com:ChristofferGreen/PrimeFrame.git" CACHE STRING "PrimeFrame git repository")
set(PRIMEFRAME_GIT_TAG "master" CACHE STRING "PrimeFrame git tag or commit")

option(PRIMESTAGE_FETCH_PRIMEHOST "Fetch PrimeHost dependency" ON)
set(PRIMEHOST_GIT_REPOSITORY "https://github.com/ChristofferGreen/PrimeHost.git" CACHE STRING "PrimeHost git repository")
set(PRIMEHOST_GIT_TAG "master" CACHE STRING "PrimeHost git tag or commit")

option(PRIMESTAGE_HEADLESS_ONLY "Build PrimeStage without PrimeManifest (no rendering)" OFF)
option(PRIMESTAGE_ENABLE_PRIMEMANIFEST "Enable PrimeManifest integration" ON)
option(PRIMESTAGE_BUILD_TESTS "Build PrimeStage tests" ON)
option(PRIMESTAGE_BUILD_EXAMPLES "Build PrimeStage example apps" OFF)
option(PRIMESTAGE_WARNINGS_AS_ERRORS "Treat warnings as errors for PrimeStage-owned targets" OFF)
option(PRIMESTAGE_ENABLE_ASAN "Enable AddressSanitizer for PrimeStage test builds" OFF)
option(PRIMESTAGE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer for PrimeStage test builds" OFF)
set(PRIMESTAGE_PRIMEMANIFEST_GIT_REPOSITORY "https://github.com/ChristofferGreen/PrimeManifest.git"
    CACHE STRING "PrimeManifest git repository")
set(PRIMESTAGE_PRIMEMANIFEST_GIT_TAG "master" CACHE STRING "PrimeManifest git tag or commit")

if((PRIMESTAGE_ENABLE_ASAN OR PRIMESTAGE_ENABLE_UBSAN) AND NOT PRIMESTAGE_BUILD_TESTS)
  message(FATAL_ERROR "Sanitizers are intended for test builds; configure with -DPRIMESTAGE_BUILD_TESTS=ON.")
endif()

if(PRIMESTAGE_FETCH_PRIMEFRAME)
  include(FetchContent)
  FetchContent_Declare(PrimeFrame
    GIT_REPOSITORY ${PRIMEFRAME_GIT_REPOSITORY}
    GIT_TAG ${PRIMEFRAME_GIT_TAG}
  )
  FetchContent_MakeAvailable(PrimeFrame)
endif()

if(PRIMESTAGE_FETCH_PRIMEHOST)
  include(FetchContent)
  set(PRIMEHOST_BUILD_TESTS OFF CACHE BOOL "Build PrimeHost tests" FORCE)
  set(PRIMEHOST_BUILD_EXAMPLES OFF CACHE BOOL "Build PrimeHost examples" FORCE)
  FetchContent_Declare(PrimeHost
    GIT_REPOSITORY ${PRIMEHOST_GIT_REPOSITORY}
    GIT_TAG ${PRIMEHOST_GIT_TAG}
  )
  FetchContent_MakeAvailable(PrimeHost)
endif()

set(CMAKE_DEBUG_POSTFIX "-debug")
set(CMAKE_RELEASE_POSTFIX "-release")
set(CMAKE_RELWITHDEBINFO_POSTFIX "-relwithdebinfo")
set(CMAKE_MINSIZEREL_POSTFIX "-minsizerel")

if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

function(ps_require_cxx23 target)
  if(CMAKE_CXX_COMPILE_FEATURES)
    target_compile_features(${target} PUBLIC cxx_std_23)
  else()
    message(WARNING "CMake lacks feature metadata for ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}; falling back to CXX_STANDARD on target ${target}.")
    set_property(TARGET ${target} PROPERTY CXX_STANDARD 23)
    set_property(TARGET ${target} PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET ${target} PROPERTY CXX_EXTENSIONS OFF)
  endif()
endfunction()

function(ps_enable_project_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(PRIMESTAGE_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
    return()
  endif()

  target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
  if(PRIMESTAGE_WARNINGS_AS_ERRORS)
    target_compile_options(${target} PRIVATE -Werror)
  endif()
endfunction()

function(ps_enable_sanitizers target)
  if(NOT (PRIMESTAGE_ENABLE_ASAN OR PRIMESTAGE_ENABLE_UBSAN))
    return()
  endif()

  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    message(FATAL_ERROR "PRIMESTAGE_ENABLE_ASAN/PRIMESTAGE_ENABLE_UBSAN requires Clang or GCC.")
  endif()

  set(sanitizer_flags)
  if(PRIMESTAGE_ENABLE_ASAN)
    list(APPEND sanitizer_flags -fsanitize=address -fno-omit-frame-pointer)
  endif()
  if(PRIMESTAGE_ENABLE_UBSAN)
    list(APPEND sanitizer_flags -fsanitize=undefined -fno-sanitize-recover=undefined)
  endif()

  target_compile_options(${target} PRIVATE ${sanitizer_flags})
  target_link_options(${target} PRIVATE ${sanitizer_flags})
endfunction()

set(PRIMESTAGE_SOURCES
  src/PrimeStage.cpp
  src/Render.cpp
)

add_library(PrimeStage ${PRIMESTAGE_SOURCES})

target_include_directories(PrimeStage
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

ps_require_cxx23(PrimeStage)
ps_enable_project_warnings(PrimeStage)
if(PRIMESTAGE_BUILD_TESTS)
  ps_enable_sanitizers(PrimeStage)
endif()

if(TARGET PrimeFrame)
  target_link_libraries(PrimeStage PUBLIC PrimeFrame)
endif()

if(NOT PRIMESTAGE_HEADLESS_ONLY AND NOT PRIMESTAGE_ENABLE_PRIMEMANIFEST)
  message(FATAL_ERROR "PrimeManifest is required unless PRIMESTAGE_HEADLESS_ONLY=ON.")
endif()

if(PRIMESTAGE_ENABLE_PRIMEMANIFEST AND NOT TARGET PrimeManifest)
  include(FetchContent)
  set(PRIMEMANIFEST_BUILD_TESTS OFF CACHE BOOL "Build PrimeManifest tests" FORCE)
  set(PRIMEMANIFEST_BUILD_EXAMPLES OFF CACHE BOOL "Build PrimeManifest examples" FORCE)
  FetchContent_Declare(
    PrimeManifest
    GIT_REPOSITORY ${PRIMESTAGE_PRIMEMANIFEST_GIT_REPOSITORY}
    GIT_TAG ${PRIMESTAGE_PRIMEMANIFEST_GIT_TAG}
  )
  FetchContent_MakeAvailable(PrimeManifest)
endif()

if(TARGET PrimeManifest)
  target_link_libraries(PrimeStage PUBLIC PrimeManifest)
  target_compile_definitions(PrimeStage PUBLIC PRIMESTAGE_HAS_PRIMEMANIFEST=1)
endif()

if(PRIMESTAGE_BUILD_TESTS)
  enable_testing()

  add_executable(PrimeStage_tests
    tests/unit/test_main.cpp
    tests/unit/test_sanity.cpp
    tests/unit/test_text_field.cpp
    tests/unit/test_text_selection.cpp
    tests/unit/test_text_selection_layout.cpp
    tests/unit/test_interaction.cpp
    tests/unit/test_tabs_dropdown.cpp
    tests/unit/test_api_ergonomics.cpp
    tests/unit/test_tree_view.cpp
    tests/unit/test_treeview_input.cpp
    tests/unit/test_focus_widgets.cpp
    tests/unit/test_focus_compositing.cpp
    tests/unit/test_input_bridge.cpp
    tests/unit/test_widget_identity.cpp
    tests/unit/test_integration_flows.cpp
    tests/unit/test_app_runtime.cpp
    tests/unit/test_visual_regression.cpp
    tests/unit/test_widgets_demo_regression.cpp
    tests/unit/test_widgets.cpp
  )
  target_link_libraries(PrimeStage_tests PRIVATE PrimeStage)
  if(TARGET PrimeHost)
    target_link_libraries(PrimeStage_tests PRIVATE PrimeHost)
  endif()
  target_include_directories(PrimeStage_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  ps_require_cxx23(PrimeStage_tests)
  ps_enable_sanitizers(PrimeStage_tests)
  add_test(NAME PrimeStage_tests COMMAND $<TARGET_FILE:PrimeStage_tests>)
endif()

if(PRIMESTAGE_BUILD_EXAMPLES)
  if(PRIMESTAGE_HEADLESS_ONLY)
    message(FATAL_ERROR "PrimeStage examples require rendering; disable PRIMESTAGE_HEADLESS_ONLY.")
  endif()
  if(NOT PRIMESTAGE_ENABLE_PRIMEMANIFEST)
    message(FATAL_ERROR "PrimeStage examples require PrimeManifest. Configure with -DPRIMESTAGE_ENABLE_PRIMEMANIFEST=ON.")
  endif()
  add_executable(primestage_example examples/primestage_example.cpp)
  target_link_libraries(primestage_example PRIVATE PrimeStage)
  target_include_directories(primestage_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  ps_require_cxx23(primestage_example)

  add_executable(primestage_widgets examples/primestage_widgets.cpp)
  target_link_libraries(primestage_widgets PRIVATE PrimeStage PrimeHost)
  target_include_directories(primestage_widgets PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  ps_require_cxx23(primestage_widgets)
endif()
