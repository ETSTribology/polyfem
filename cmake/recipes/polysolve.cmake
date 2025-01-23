# PolySolve (https://github.com/polyfem/polysolve)
# License: MIT

# polysolve.cmake

# 1. Include Guard to Prevent Multiple Inclusions
if(POLYSOLVE_INCLUDED)
    return()
endif()
set(POLYSOLVE_INCLUDED TRUE)

# 2. Check if the Alias Already Exists
if(TARGET polysolve::polysolve)
    message(STATUS "Target 'polysolve::polysolve' already exists. Skipping PolySolve setup.")
    return()
endif()

message(STATUS "Third-party: creating target 'polysolve::polysolve'")

# 3. Parameterize Local Path
set(POLYSOLVE_LOCAL_PATH "${POLYSOLVE_LOCAL_PATH}" CACHE PATH "Path to local PolySolve source")

# Attempt to find a system or local installation first
find_package(polysolve QUIET 
    HINTS ${POLYSOLVE_DIR} ${CMAKE_PREFIX_PATH}
    PATH_SUFFIXES lib/cmake/polysolve
)

if(polysolve_FOUND)
    message(STATUS "Found system or local PolySolve: ${polysolve_DIR}")
    if(TARGET polysolve)
        add_library(polysolve::polysolve ALIAS polysolve)
    else()
        message(FATAL_ERROR "PolySolve was found but the 'polysolve' target does not exist.")
    endif()
else()
    # Fall Back to CPM with Improved Dependency Handling
    include(CPM)
    CPMAddPackage(
        NAME polysolve
        GIT_REPOSITORY "https://github.com/polyfem/polysolve.git"
        GIT_TAG "768ee9cbe2de91e47c558e5a2f1be3dec0bfe561"
        OPTIONS 
            "POLYSOLVE_WITH_CUSOLVER=${POLYSOLVE_WITH_CUSOLVER}"
            "POLYSOLVE_WITH_AMGCL=${POLYSOLVE_WITH_AMGCL}"
            "POLYSOLVE_WITH_HYPRE=${POLYSOLVE_WITH_HYPRE}"
            "POLYSOLVE_WITH_SUPERLU=${POLYSOLVE_WITH_SUPERLU}"
            "POLYSOLVE_WITH_CHOLMOD=${POLYSOLVE_WITH_CHOLMOD}"
            "POLYSOLVE_WITH_UMFPACK=${POLYSOLVE_WITH_UMFPACK}"
            "SUITESPARSE_USE_CUDA=${SUITESPARSE_USE_CUDA}"
            "SUITESPARSE_CUDA_ARCHITECTURES=${SUITESPARSE_CUDA_ARCHITECTURES}"
            "CMAKE_CUDA_ARCHITECTURES=${CMAKE_CUDA_ARCHITECTURES}"
            "CMAKE_CUDA_COMPILER=${CMAKE_CUDA_COMPILER}"
            "CUDA_TOOLKIT_ROOT_DIR=${CUDA_TOOLKIT_ROOT_DIR}"
            "SuperLU_DIR=${SuperLU_DIR}"
            "SuiteSparse_DIR=${SuiteSparse_DIR}"
    )

    if(polysolve_ADDED)
        # Configure the Actual Target First
        if(POLYSOLVE_WITH_CUSOLVER)
            find_package(CUDAToolkit REQUIRED)
            if(TARGET CUDA::cusolver)
                target_link_libraries(polysolve PUBLIC CUDA::cusolver)
            else()
                message(WARNING "CUSOLVER not found - CUDA support disabled for PolySolve.")
                set(POLYSOLVE_WITH_CUSOLVER OFF CACHE BOOL "" FORCE)
            endif()
        endif()

        # Handle OpenMP Dependencies
        if(POLYFEM_THREADING STREQUAL "OpenMP")
            find_package(OpenMP REQUIRED)
            if(OpenMP_CXX_FOUND)
                target_link_libraries(polysolve PUBLIC OpenMP::OpenMP_CXX)
                target_compile_definitions(polysolve PUBLIC POLYFEM_USE_OPENMP)
            else()
                message(WARNING "OpenMP not found - OpenMP support disabled for PolySolve.")
                set(POLYFEM_THREADING "NONE" CACHE STRING "Threading backend" FORCE)
            endif()
        endif()

        # Handle SUPERLU Dependencies
        if(POLYSOLVE_WITH_SUPERLU)
            find_package(SuperLU QUIET)
            if(SuperLU_FOUND)
                target_link_libraries(polysolve PUBLIC SuperLU::SuperLU)
            else()
                message(WARNING "SUPERLU not found - SuperLU support disabled for PolySolve.")
                set(POLYSOLVE_WITH_SUPERLU OFF CACHE BOOL "" FORCE)
            endif()
        endif()

        # Create Alias After Configuring the Real Target
        if(NOT TARGET polysolve::polysolve)
            add_library(polysolve::polysolve ALIAS polysolve)
            message(STATUS "PolySolve built from CPM source")
        endif()
    else()
        message(FATAL_ERROR "PolySolve build failed - check dependency paths")
    endif()
endif()

# Final Verification
if(NOT TARGET polysolve::polysolve)
    message(FATAL_ERROR "PolySolve target creation failed! Missing dependencies: "
        "SuperLU: ${SuperLU_FOUND} "
        "SuiteSparse: ${SuiteSparse_FOUND} "
        "OpenMP: ${OpenMP_CXX_FOUND}")
endif()
