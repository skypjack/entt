# Testbed configuration

include(FetchContent)

# Fetch SDL3

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL
    GIT_TAG release-3.2.8
    GIT_SHALLOW 1
)

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

set(ENTT_TESTBED_EXECUTABLE testbed)

add_executable(${ENTT_TESTBED_EXECUTABLE})

set_target_properties(${ENTT_TESTBED_EXECUTABLE} PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(${ENTT_TESTBED_EXECUTABLE} PUBLIC ${ENTT_CXX_STD})

target_compile_definitions(
    ${ENTT_TESTBED_EXECUTABLE}
    PRIVATE
        ENTT_ID_TYPE=${ENTT_ID_TYPE}
        NOMINMAX
)

target_sources(
	${ENTT_TESTBED_EXECUTABLE}
	PRIVATE
		testbed.cpp
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl3.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer3.cpp
)

target_link_libraries(
    ${ENTT_TESTBED_EXECUTABLE}
    PRIVATE
        EnTT::EnTT
        SDL3::SDL3
)

target_include_directories(
    ${ENTT_TESTBED_EXECUTABLE}
    PRIVATE
        ${testbed_SOURCE_DIR}
        ${imgui_SOURCE_DIR}
)
