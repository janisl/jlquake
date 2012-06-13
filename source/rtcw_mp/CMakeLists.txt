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
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp

	client/snd_dma.cpp

	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp

	server/sv_bot.cpp
	server/sv_ccmds.cpp
	server/sv_client.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp

	game/q_math.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(WOLFMP_NASM_FILES
		unix/snapvector.nasm
	)
	foreach(SRC ${WOLFMP_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		list(APPEND WOLFMP_FILES ${OBJ})
	endforeach(SRC)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlwolfmp ${WOLFMP_FILES})
set_target_properties(jlwolfmp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfmp ${CLIENTS_LIBS})

set(WOLFMPDED_FILES
	null/null_client.cpp
	null/null_snddma.cpp
	server/sv_bot.cpp
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
	game/q_math.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(WOLFMPDED_NASM_FILES
		unix/snapvector.nasm
	)
	foreach(SRC ${WOLFMPDED_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		list(APPEND WOLFMPDED_FILES ${OBJ})
	endforeach(SRC)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlwolfmpded ${WOLFMPDED_FILES})
set_target_properties(jlwolfmpded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O -DDEDICATED -DC_ONLY")
target_link_libraries(jlwolfmpded ${SERVERS_LIBS} m)
