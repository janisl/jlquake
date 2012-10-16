add_executable(jlquake
	cl_main.cpp
	common.cpp
	host.cpp
	host_cmd.cpp
	sys_linux.cpp
)
set_target_properties(jlquake PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake ${CLIENTS_LIBS})

add_executable(jlquakeded
	common.cpp
	host.cpp
	host_cmd.cpp
	sys_linux.cpp
)
set_target_properties(jlquakeded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlquakeded ${SERVERS_LIBS})
