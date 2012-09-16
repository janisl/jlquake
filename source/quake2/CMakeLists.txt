add_executable(jlquake2
	client/cl_cin.cpp
	client/cl_ents.cpp
	client/cl_input.cpp
	client/cl_inv.cpp
	client/cl_main.cpp
	client/cl_parse.cpp
	client/cl_pred.cpp
	client/cl_scrn.cpp
	client/cl_view.cpp
	client/console.cpp
	client/keys.cpp
	client/snd_dma.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp
	qcommon/q_shared.cpp
	linux/sys_linux.cpp
)
set_target_properties(jlquake2 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake2 ${CLIENTS_LIBS})

add_executable(jlquake2ded
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp
	qcommon/q_shared.cpp
	linux/sys_linux.cpp
	null/cl_null.cpp
)
set_target_properties(jlquake2ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED_ONLY")
target_link_libraries(jlquake2ded ${SERVERS_LIBS})
