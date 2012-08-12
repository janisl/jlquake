add_executable(jlquake
	cl_demo.cpp
	cl_draw.cpp
	cl_input.cpp
	cl_main.cpp
	cl_parse.cpp
	cl_screen.cpp
	cmd.cpp
	common.cpp
	console.cpp
	cvar.cpp
	host.cpp
	host_cmd.cpp
	keys.cpp
	menu.cpp
	net_main.cpp
	r_part.cpp
	sbar.cpp
	snd_dma.cpp
	sv_user.cpp
	sys_linux.cpp
	view.cpp
)
set_target_properties(jlquake PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake ${CLIENTS_LIBS})

add_executable(jlquakeded
	cmd.cpp
	common.cpp
	console.cpp
	cvar.cpp
	host.cpp
	host_cmd.cpp
	net_main.cpp
	sv_user.cpp
	sys_linux.cpp
)
set_target_properties(jlquakeded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlquakeded ${SERVERS_LIBS})
