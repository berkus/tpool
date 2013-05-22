cmake_minimum_required(VERSION 2.8)
project(tpool)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11 -stdlib=libc++")

enable_testing()
include(Dart)
include(CTest)

set(Boost_USE_MULTITHREAD ON)
find_package(Boost REQUIRED COMPONENTS unit_test_framework)

# Create and link a test application.
function(create_test NAME EXTRALIBS)
	add_executable(test_${NAME} ${NAME}_test.cpp)
	target_link_libraries(test_${NAME} ${EXTRALIBS} ${Boost_LIBRARIES})
	install(TARGETS test_${NAME}
	    RUNTIME DESTINATION tests)
	add_test(${NAME} test_${NAME})
endfunction(create_test)

create_test(thread_pool "")
