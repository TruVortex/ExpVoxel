cmake_minimum_required(VERSION 3.20)
project(expvoxel CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64|i386|i686")
    if(MSVC)
        add_compile_options(/arch:AVX2)
    else()
        add_compile_options(-mavx2)
    endif()
endif()

include(FetchContent)
FetchContent_Declare(
    tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG        v0.10
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(tracy)

find_package(SDL2 REQUIRED)

add_library(core
    src/concurrency/thread_pool.cpp
    src/concurrency/work_stealing_pool.cpp
    src/rendering/dda.cpp
    src/rendering/simd_dda.cpp
    src/rendering/tile_renderer.cpp
)
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(core PUBLIC Tracy::TracyClient)

add_executable(expvoxel src/main.cpp)
target_link_libraries(expvoxel PRIVATE core SDL2::SDL2)

enable_testing()
add_executable(scalar_reference_tests tests/scalar_reference_tests.cpp)
target_link_libraries(scalar_reference_tests PRIVATE core)
add_test(NAME ScalarReferenceTests COMMAND scalar_reference_tests)

add_executable(allocator_tests tests/allocator_tests.cpp)
target_link_libraries(allocator_tests PRIVATE core)
add_test(NAME AllocatorTests COMMAND allocator_tests)

add_executable(simd_equivalence_tests tests/simd_equivalence_tests.cpp)
target_link_libraries(simd_equivalence_tests PRIVATE core)
add_test(NAME SimdEquivalenceTests COMMAND simd_equivalence_tests)

add_executable(multithread_equivalence_tests tests/multithread_equivalence_tests.cpp)
target_link_libraries(multithread_equivalence_tests PRIVATE core)
add_test(NAME MultithreadEquivalenceTests COMMAND multithread_equivalence_tests)

add_executable(scheduler_stress_tests tests/scheduler_stress_tests.cpp)
target_link_libraries(scheduler_stress_tests PRIVATE core)
add_test(NAME SchedulerStressTests COMMAND scheduler_stress_tests)

add_executable(epoch_tests tests/epoch_tests.cpp)
target_link_libraries(epoch_tests PRIVATE core)
add_test(NAME EpochTests COMMAND epoch_tests)

add_executable(fuzz_tests tests/fuzz_tests.cpp)
target_link_libraries(fuzz_tests PRIVATE core)
add_test(NAME FuzzTests COMMAND fuzz_tests)

add_executable(bench_engine benchmarks/bench_engine.cpp)
target_link_libraries(bench_engine PRIVATE core)