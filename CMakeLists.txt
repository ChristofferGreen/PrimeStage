cmake_minimum_required(VERSION 3.20)
project(PrimeStage VERSION 0.1.0 LANGUAGES CXX)

option(PRIMESTAGE_FETCH_PRIMEFRAME "Fetch PrimeFrame dependency" ON)
set(PRIMEFRAME_GIT_REPOSITORY "git@github.com:ChristofferGreen/PrimeFrame.git" CACHE STRING "PrimeFrame git repository")
set(PRIMEFRAME_GIT_TAG "master" CACHE STRING "PrimeFrame git tag or commit")

option(PRIMESTAGE_ENABLE_PRIMEMANIFEST "Enable PrimeManifest integration" OFF)
set(PRIMESTAGE_PRIMEMANIFEST_GIT_REPOSITORY "https://github.com/ChristofferGreen/PrimeManifest.git"
    CACHE STRING "PrimeManifest git repository")
set(PRIMESTAGE_PRIMEMANIFEST_GIT_TAG "master" CACHE STRING "PrimeManifest git tag or commit")

if(PRIMESTAGE_FETCH_PRIMEFRAME)
  include(FetchContent)
  FetchContent_Declare(PrimeFrame
    GIT_REPOSITORY ${PRIMEFRAME_GIT_REPOSITORY}
    GIT_TAG ${PRIMEFRAME_GIT_TAG}
  )
  FetchContent_MakeAvailable(PrimeFrame)
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

if(TARGET PrimeFrame)
  target_link_libraries(PrimeStage PUBLIC PrimeFrame)
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

option(PRIMESTAGE_BUILD_TESTS "Build PrimeStage tests" ON)
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
    tests/unit/test_tree_view.cpp
    tests/unit/test_treeview_input.cpp
    tests/unit/test_widgets.cpp
  )
  target_link_libraries(PrimeStage_tests PRIVATE PrimeStage)
  target_include_directories(PrimeStage_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  ps_require_cxx23(PrimeStage_tests)
  add_test(NAME PrimeStage_tests COMMAND $<TARGET_FILE:PrimeStage_tests>)
endif()

option(PRIMESTAGE_BUILD_EXAMPLES "Build PrimeStage example apps" OFF)
if(PRIMESTAGE_BUILD_EXAMPLES)
  add_executable(primestage_example examples/primestage_example.cpp)
  target_link_libraries(primestage_example PRIVATE PrimeStage)
  target_include_directories(primestage_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  ps_require_cxx23(primestage_example)
endif()
