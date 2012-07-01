#############################################################################
# CLIENT/SERVER
#############################################################################

set(QUAKE3_FILES
	client/cl_cgame.cpp
	client/cl_cin.cpp
	client/cl_console.cpp
	client/cl_input.cpp
	client/cl_keys.cpp
	client/cl_main.cpp
	client/cl_net_chan.cpp
	client/cl_parse.cpp
	client/cl_scrn.cpp
	client/cl_ui.cpp
	client/snd_dma.cpp
	server/sv_ccmds.cpp
	server/sv_client.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

add_executable(jlquake3 ${QUAKE3_FILES})
set_target_properties(jlquake3 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlquake3 ${CLIENTS_LIBS})

#############################################################################
# DEDICATED SERVER                        
#############################################################################

set(QUAKE3_DEDICATED_FILES
	server/sv_client.cpp
	server/sv_ccmds.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp
	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	null/null_client.cpp
	null/null_snddma.cpp
)

add_executable(jlquake3ded ${QUAKE3_DEDICATED_FILES})
set_target_properties(jlquake3ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED -DC_ONLY")
target_link_libraries(jlquake3ded ${SERVERS_LIBS})
