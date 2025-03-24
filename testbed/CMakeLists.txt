# Testbed configuration

add_executable(testbed)

set(ENTT_TESTBED_EXECUTABLE testbed)

set_target_properties(testbed PROPERTIES CXX_EXTENSIONS OFF)
target_compile_features(testbed PUBLIC ${ENTT_CXX_STD})

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
)

target_link_libraries(
    ${ENTT_TESTBED_EXECUTABLE}
    PRIVATE
        EnTT::EnTT
)

target_include_directories(
    ${ENTT_TESTBED_EXECUTABLE}
    PRIVATE
        ${testbed_SOURCE_DIR}
)
