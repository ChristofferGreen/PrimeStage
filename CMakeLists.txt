cmake_minimum_required(VERSION 3.20)
project(PrimeStage VERSION 0.1.0 LANGUAGES CXX)

option(PRIMESTAGE_FETCH_PRIMEFRAME "Fetch PrimeFrame dependency" ON)
set(PRIMEFRAME_GIT_REPOSITORY "git@github.com:ChristofferGreen/PrimeFrame.git" CACHE STRING "PrimeFrame git repository")
set(PRIMEFRAME_GIT_TAG "master" CACHE STRING "PrimeFrame git tag or commit")

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

option(PRIMESTAGE_BUILD_TESTS "Build PrimeStage tests" ON)
if(PRIMESTAGE_BUILD_TESTS)
  enable_testing()

  add_executable(PrimeStage_tests
    tests/unit/test_main.cpp
    tests/unit/test_builder.cpp
    tests/unit/test_sanity.cpp
  )
  target_link_libraries(PrimeStage_tests PRIVATE PrimeStage)
  target_include_directories(PrimeStage_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  ps_require_cxx23(PrimeStage_tests)
  add_test(NAME PrimeStage_tests COMMAND $<TARGET_FILE:PrimeStage_tests>)
endif()

option(PRIMESTAGE_BUILD_EXAMPLES "Build PrimeStage example apps" OFF)
if(PRIMESTAGE_BUILD_EXAMPLES)
  include(FetchContent)

  option(PRIMESTAGE_PRIMEMANIFEST_ENABLE_FONTS "Enable PrimeManifest font rendering for examples" ON)
  set(PRIMESTAGE_PRIMEMANIFEST_GIT_TAG "master" CACHE STRING "PrimeManifest git tag or commit")

  set(PRIMEMANIFEST_BUILD_TESTS OFF CACHE BOOL "Build PrimeManifest tests" FORCE)
  set(PRIMEMANIFEST_ENABLE_FONTS ${PRIMESTAGE_PRIMEMANIFEST_ENABLE_FONTS} CACHE BOOL "Enable PrimeManifest fonts" FORCE)
  set(PRIMEMANIFEST_BUILD_EXAMPLES OFF CACHE BOOL "Build PrimeManifest examples" FORCE)

  FetchContent_Declare(
    PrimeManifest
    GIT_REPOSITORY https://github.com/ChristofferGreen/PrimeManifest.git
    GIT_TAG ${PRIMESTAGE_PRIMEMANIFEST_GIT_TAG}
  )
  FetchContent_MakeAvailable(PrimeManifest)

  if(TARGET PrimeManifest)
    target_link_libraries(PrimeStage PUBLIC PrimeManifest)
    target_compile_definitions(PrimeStage PUBLIC PRIMESTAGE_HAS_PRIMEMANIFEST=1)
  endif()

  add_executable(primestage_render_demo examples/primestage_render_demo.cpp)
  target_link_libraries(primestage_render_demo PRIVATE PrimeStage PrimeFrame PrimeManifest)
  target_include_directories(primestage_render_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  if(DEFINED primemanifest_SOURCE_DIR)
    file(GLOB_RECURSE PRIMESTAGE_PM_FONTS
      "${primemanifest_SOURCE_DIR}/*.ttf"
      "${primemanifest_SOURCE_DIR}/*.otf"
      "${primemanifest_SOURCE_DIR}/*.ttc"
      "${primemanifest_SOURCE_DIR}/*.otc"
    )
    if(PRIMESTAGE_PM_FONTS)
      list(GET PRIMESTAGE_PM_FONTS 0 PRIMESTAGE_PM_FONT_SAMPLE)
      get_filename_component(PRIMESTAGE_PM_FONT_DIR "${PRIMESTAGE_PM_FONT_SAMPLE}" DIRECTORY)
      target_compile_definitions(primestage_render_demo PRIVATE
        PRIMESTAGE_BUNDLED_FONT_DIR=\"${PRIMESTAGE_PM_FONT_DIR}\"
        PRIMESTAGE_HAS_BUNDLED_FONT=1
      )
    else()
      target_compile_definitions(primestage_render_demo PRIVATE PRIMESTAGE_HAS_BUNDLED_FONT=0)
    endif()
  else()
    target_compile_definitions(primestage_render_demo PRIVATE PRIMESTAGE_HAS_BUNDLED_FONT=0)
  endif()
  ps_require_cxx23(primestage_render_demo)
endif()
