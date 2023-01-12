cmake_minimum_required(VERSION 3.0.0)
project(metabm VERSION 0.1.0)

SET(CMAKE_BUILD_TYPE "Release")
if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3")
    message(STATUS "CMAKE_BUILD_TYPE = Release")
endif()
SET(CMAKE_CXX_STANDARD 17)

add_executable(umap_bmtest unordered_map.cc utils.cc)
target_link_libraries(umap_bmtest pthread)

add_executable(ankerl_hashmap_bmtest ankerl_hashmap.cc utils.cc)
find_package(unordered_dense CONFIG REQUIRED)
target_link_libraries(ankerl_hashmap_bmtest pthread unordered_dense::unordered_dense)

add_executable(emhash8_bmtest emhash8.cc utils.cc)
# target_compile_definitions(emhash8_bmtest PUBLIC EMH_HIGH_LOAD) # 别开这个宏，开了跑不出结果了
target_link_libraries(emhash8_bmtest pthread)

add_executable(radix_tree_bmtest radix_tree.cc utils.cc 3rdparty/mcatfs/common.cpp)
target_link_libraries(radix_tree_bmtest pthread numa)