cmake_minimum_required(VERSION 3.18)
project(HamiltonianPathWithStops
    LANGUAGES CXX
    DESCRIPTION "Min-max subpath optimization in Hamiltonian paths with stops"
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ─── nlohmann/json ───────────────────────────────────────────────────────────
find_package(nlohmann_json QUIET)
if(NOT nlohmann_json_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG        v3.11.3
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(nlohmann_json)
    message(STATUS "nlohmann/json fetched via FetchContent")
else()
    message(STATUS "nlohmann/json found on system")
endif()

# ─── Header-only utils library ───────────────────────────────────────────────
add_library(utils INTERFACE)
target_include_directories(utils INTERFACE ${CMAKE_SOURCE_DIR})
target_link_libraries(utils INTERFACE nlohmann_json::nlohmann_json)

# ─── Sequential targets ─────────────────────────────────────────────────────
add_executable(exact_seq     sequential/exact_seq.cpp)
add_executable(brute_seq     sequential/brute_seq.cpp)
add_executable(greedy_seq    sequential/greedy_seq.cpp)
add_executable(greedy_rand_seq sequential/greedy_rand_seq.cpp)
add_executable(genetic_seq   sequential/genetic_seq.cpp)

target_link_libraries(exact_seq      PRIVATE utils)
target_link_libraries(brute_seq      PRIVATE utils)
target_link_libraries(greedy_seq     PRIVATE utils)
target_link_libraries(greedy_rand_seq PRIVATE utils)
target_link_libraries(genetic_seq    PRIVATE utils)

# ─── OpenMP targets (optional) ──────────────────────────────────────────────
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found — building parallel targets")

    add_executable(exact_par   parallel/exact_par.cpp)
    add_executable(brute_par   parallel/brute_par.cpp)
    add_executable(greedy_par  parallel/greedy_par.cpp)
    add_executable(genetic_par parallel/genetic_par.cpp)

    target_link_libraries(exact_par   PRIVATE utils OpenMP::OpenMP_CXX)
    target_link_libraries(brute_par   PRIVATE utils OpenMP::OpenMP_CXX)
    target_link_libraries(greedy_par  PRIVATE utils OpenMP::OpenMP_CXX)
    target_link_libraries(genetic_par PRIVATE utils OpenMP::OpenMP_CXX)

    # Test case batch generator also uses OpenMP
    add_executable(serialgnp testcase_generation/serialgnp.cpp)
    target_link_libraries(serialgnp PRIVATE OpenMP::OpenMP_CXX)
else()
    message(WARNING "OpenMP not found — skipping parallel targets")
endif()

# ─── CUDA targets (optional) ────────────────────────────────────────────────
include(CheckLanguage)
check_language(CUDA)
if(CMAKE_CUDA_COMPILER)
    enable_language(CUDA)
    set(CMAKE_CUDA_STANDARD 17)
    set(CMAKE_CUDA_STANDARD_REQUIRED ON)
    message(STATUS "CUDA found — building GPU targets")

    add_executable(exact_cuda_v parallel_cuda/exact_par_vertices.cu)
    add_executable(exact_cuda_e parallel_cuda/exact_par_edges.cu)

    target_include_directories(exact_cuda_v PRIVATE ${CMAKE_SOURCE_DIR})
    target_include_directories(exact_cuda_e PRIVATE ${CMAKE_SOURCE_DIR})
    target_link_libraries(exact_cuda_v PRIVATE nlohmann_json::nlohmann_json)
    target_link_libraries(exact_cuda_e PRIVATE nlohmann_json::nlohmann_json)
else()
    message(WARNING "CUDA not found — skipping GPU targets")
endif()

# ─── Test case generator ───────────────────────────────────────────────────
add_executable(gnp testcase_generation/gnp.cpp)
target_link_libraries(gnp PRIVATE nlohmann_json::nlohmann_json)
target_include_directories(gnp PRIVATE ${CMAKE_SOURCE_DIR})
