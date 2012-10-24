include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

#############################################################################
# SERVER
#############################################################################

add_executable(jlqwsv
	server/sv_main.cpp
	server/sys_unix.cpp
	client/common.cpp
)
set_target_properties(jlqwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DSERVERONLY")
target_link_libraries(jlqwsv ${SERVERS_LIBS})

#############################################################################
# CLIENT
#############################################################################

add_executable(jlqwcl
	client/cl_main.cpp
	client/common.cpp
	client/sys_linux.cpp
)
set_target_properties(jlqwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlqwcl ${CLIENTS_LIBS})
