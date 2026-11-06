FetchContent_Declare(
    cereal
    GIT_REPOSITORY https://github.com/USCiLab/cereal.git
    GIT_TAG v1.3.2
    GIT_SHALLOW 1
)

 set(BUILD_DOC OFF CACHE BOOL "" FORCE)
 set(BUILD_SANDBOX OFF CACHE BOOL "" FORCE)
 set(SKIP_PORTABILITY_TEST ON CACHE BOOL "" FORCE)
 set(SKIP_PERFORMANCE_COMPARISON ON CACHE BOOL "" FORCE)

 FetchContent_MakeAvailable(cereal)
 