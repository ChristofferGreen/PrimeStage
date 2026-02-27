cmake_minimum_required(VERSION 3.20)
project(PrimeStageFindPackageSmoke LANGUAGES CXX)

find_package(PrimeStage CONFIG REQUIRED)

add_executable(primestage_find_package_smoke main.cpp)
target_link_libraries(primestage_find_package_smoke PRIVATE PrimeStage::PrimeStage)

target_compile_features(primestage_find_package_smoke PRIVATE cxx_std_23)
