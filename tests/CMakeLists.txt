file (
    GLOB TEST_FILES
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    "*.cc"
)

add_executable ( tests ${TEST_FILES} )
target_link_libraries ( tests PUBLIC ltl )
