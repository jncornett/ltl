cmake_minimum_required ( VERSION 3.2 )
project ( Ltl )

include ( FindLuaJIT.cmake )

set ( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -ggdb" )

add_subdirectory ( src )
add_subdirectory ( demo )
add_subdirectory ( tests )
