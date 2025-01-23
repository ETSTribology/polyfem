# IPC Toolkit (https://github.com/ipc-sim/ipc-toolkit)
# License: MIT

# ipc_toolkit.cmake

# 1. Include Guard to Prevent Multiple Inclusions
if(IPC_TOOLKIT_INCLUDED)
    return()
endif()
set(IPC_TOOLKIT_INCLUDED TRUE)

# 2. Check if the Alias Already Exists
if(TARGET ipc::toolkit)
    message(STATUS "Target 'ipc::toolkit' already exists. Skipping IPC Toolkit setup.")
    return()
endif()

message(STATUS "Third-party: creating target 'ipc::toolkit'")

# 3. Parameterize Local Path
set(IPC_TOOLKIT_LOCAL_PATH "${IPC_TOOLKIT_LOCAL_PATH}" CACHE PATH "Path to local IPC Toolkit source")

if(EXISTS "${IPC_TOOLKIT_LOCAL_PATH}/CMakeLists.txt")
    message(STATUS "Using local IPC Toolkit at: ${IPC_TOOLKIT_LOCAL_PATH}")
    
    # Add as a subdirectory to inherit build settings
    add_subdirectory("${IPC_TOOLKIT_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/ipc_toolkit-local")
    
    # Create Alias After Building the Actual Target
    if(TARGET ipc_toolkit)
        if(NOT TARGET ipc::toolkit)
            add_library(ipc::toolkit ALIAS ipc_toolkit)
        else()
            message(STATUS "Alias 'ipc::toolkit' already exists. Skipping alias creation.")
        endif()
    else()
        message(FATAL_ERROR "Expected target 'ipc_toolkit' not found in local IPC Toolkit.")
    endif()

else()
    # Attempt to Find System Installation
    find_package(ipc_toolkit QUIET 
        HINTS ${IPC_TOOLKIT_DIR} ${CMAKE_PREFIX_PATH}
        PATH_SUFFIXES lib/cmake/ipc_toolkit
    )

    if(ipc_toolkit_FOUND)
        message(STATUS "Found system IPC Toolkit: ${ipc_toolkit_DIR}")
        if(NOT TARGET ipc::toolkit)
            add_library(ipc::toolkit ALIAS ipc_toolkit)
        else()
            message(STATUS "Alias 'ipc::toolkit' already exists. Skipping alias creation.")
        endif()
    else()
        # Fall Back to CPM (CMake Package Manager)
        include(CPM)
        CPMAddPackage(
            NAME ipc_toolkit
            GIT_REPOSITORY "https://github.com/ipc-sim/ipc-toolkit.git"
            GIT_TAG "617b80e92eb4310791a722fd1cc20c902b17683f"
            OPTIONS
                "IPC_TOOLKIT_BUILD_TESTS OFF"
                "IPC_TOOLKIT_BUILD_APPS OFF"
                "IPC_TOOLKIT_WITH_CUDA ${IPC_TOOLKIT_WITH_CUDA}"
        )
        
        if(ipc_toolkit_ADDED)
            if(IPC_TOOLKIT_WITH_CUDA)
                find_package(CUDAToolkit REQUIRED)
                if(TARGET CUDA::curand AND TARGET CUDA::cusolver)
                    target_link_libraries(ipc_toolkit PUBLIC 
                        CUDA::curand
                        CUDA::cusolver
                    )
                    target_compile_definitions(ipc_toolkit PUBLIC IPC_TOOLKIT_WITH_CUDA)
                else()
                    message(WARNING "CUDA Toolkit components not found. Disabling CUDA support for IPC Toolkit.")
                    set(IPC_TOOLKIT_WITH_CUDA OFF CACHE BOOL "" FORCE)
                endif()
            endif()

            # Create Alias After Configuring the Real Target
            if(NOT TARGET ipc::toolkit)
                add_library(ipc::toolkit ALIAS ipc_toolkit)
                message(STATUS "IPC Toolkit built from CPM source")
            else()
                message(STATUS "Alias 'ipc::toolkit' already exists. Skipping alias creation.")
            endif()
        else()
            message(FATAL_ERROR "Failed to find or build IPC Toolkit")
        endif()
    endif()
endif()

# 4. Final Verification
if(NOT TARGET ipc::toolkit)
    message(FATAL_ERROR "IPC Toolkit target creation failed!")
endif()
