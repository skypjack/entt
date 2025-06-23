# Testbed configuration

include(FetchContent)

# Fetch SDL3

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-3.2.10
    GIT_SHALLOW 1
)

option(SDL_STATIC "Enable SDL static library" ON)
option(SDL_SHARED "Disable SDL shared library" OFF)

FetchContent_MakeAvailable(SDL3)

# Fetch ImGui

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG v1.91.9
    GIT_SHALLOW 1
)

FetchContent_GetProperties(imgui)

if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
    set(imgui_INCLUDE_DIR ${imgui_SOURCE_DIR})
endif()

# Testbed executable

add_executable(testbed)

set_target_properties(testbed PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(testbed PUBLIC ${ENTT_CXX_STD})

target_compile_definitions(
    testbed
    PRIVATE
        ENTT_ID_TYPE=${ENTT_ID_TYPE}
        NOMINMAX
)

target_sources(
	testbed
	PRIVATE
        application/application.cpp
        application/context.cpp
        meta/meta.cpp
        system/hud_system.cpp
        system/imgui_system.cpp
        system/input_system.cpp
        system/rendering_system.cpp
        testbed.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_link_libraries(
    testbed
    PRIVATE
        EnTT::EnTT
        SDL3::SDL3
)

target_include_directories(
    testbed
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
)
