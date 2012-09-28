add_executable(jlhexen2
	cl_demo.cpp
	cl_draw.cpp
	cl_input.cpp
	cl_main.cpp
	cl_parse.cpp
	cl_screen.cpp
	common.cpp
	console.cpp
	host.cpp
	host_cmd.cpp
	keys.cpp
	net_main.cpp
	r_part.cpp
	view.cpp
	sys_linux.cpp
	snd_dma.cpp
)
set_target_properties(jlhexen2 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlhexen2 ${CLIENTS_LIBS})

add_executable(jlhexen2ded
	common.cpp
	console.cpp
	host.cpp
	host_cmd.cpp
	net_main.cpp
	sys_linux.cpp
)
set_target_properties(jlhexen2ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlhexen2ded ${SERVERS_LIBS})
