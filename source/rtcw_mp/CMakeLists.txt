#  $DEDICATED_NAME = 'wolfupdate.' . $cpu;

#  $COMMON_CFLAGS = '-pipe -fsigned-char -DPRE_RELEASE_DEMO ';
#  $BASEGAME = 'demomain';
#  $COMMON_CFLAGS = '-pipe -fsigned-char -DDO_LIGHT_DEDICATED ';
#  $BASEGAME = 'main';
#  $COMMON_CFLAGS = '-pipe -fsigned-char -DUPDATE_SERVER ';
#  $BASEGAME = 'main';
#  $COMMON_CFLAGS .= "-DAUTOUPDATE_SERVER_NAME=\\\"$update_server\\\" ";

# NOTE TTimo using -fshort-enums increases warnings on enum issues
#   this is for debugging and QA ONLY, the flag has ABI issues (OpenGL headers)
#$COMMON_CFLAGS = $COMMON_CFLAGS . '-fshort-enums ';

set(WOLFMP_FILES
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

	qcommon/cmd.cpp 
	qcommon/common.cpp
	qcommon/files.cpp

	client/snd_dma.cpp

	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

add_executable(jlwolfmp ${WOLFMP_FILES})
set_target_properties(jlwolfmp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfmp ${CLIENTS_LIBS})

set(WOLFMPDED_FILES
	null/null_client.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/files.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

add_executable(jlwolfmpded ${WOLFMPDED_FILES})
set_target_properties(jlwolfmpded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O -DDEDICATED -DC_ONLY")
target_link_libraries(jlwolfmpded ${SERVERS_LIBS} m)
