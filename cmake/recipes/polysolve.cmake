# PolySolve (https://github.com/polyfem/polysolve)
# License: MIT

if(TARGET polysolve)
    return()
endif()

message(STATUS "Third-party: creating target 'polysolve'")

include(CPM)
CPMAddPackage("gh:ETSTribology/polysolve#37f28368bf6264e7d5a316dc69c3d77179fa2e76")
