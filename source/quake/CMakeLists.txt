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
	pr_cmds.cpp
	r_part.cpp
	sbar.cpp
	snd_dma.cpp
	sv_main.cpp
	sv_user.cpp
	sys_linux.cpp
	view.cpp
	zone.cpp
)
set_target_properties(jlquake PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake ${CLIENTS_LIBS})
