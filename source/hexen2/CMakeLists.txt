add_executable(jlhexen2
	cl_demo.cpp
	cl_input.cpp
	cl_main.cpp
	cl_parse.cpp
	common.cpp
	host.cpp
	host_cmd.cpp
	keys.cpp
	net_main.cpp
	sys_linux.cpp
)
set_target_properties(jlhexen2 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlhexen2 ${CLIENTS_LIBS})

add_executable(jlhexen2ded
	common.cpp
	host.cpp
	host_cmd.cpp
	keys.cpp
	net_main.cpp
	sys_linux.cpp
)
set_target_properties(jlhexen2ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlhexen2ded ${SERVERS_LIBS})
