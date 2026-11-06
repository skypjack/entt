FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-3.4.20
    GIT_SHALLOW 
    1
)

option(SDL_STATIC "Enable SDL static library" ON)
option(SDL_SHARED "Disable SDL shared library" OFF)

FetchContent_MakeAvailable(SDL3)
