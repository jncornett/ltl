add_library ( ltl SHARED ltl.h )
set_target_properties ( ltl PROPERTIES LINKER_LANGUAGE CXX )
target_include_directories ( ltl PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
