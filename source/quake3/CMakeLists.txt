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
	server/sv_bot.cpp
	server/sv_ccmds.cpp
	server/sv_client.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	server/sv_world.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	qcommon/vm.cpp
	qcommon/vm_interpreted.cpp
	game/q_shared.cpp
	game/q_math.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(QUAKE3_FILES ${QUAKE3_FILES}
		qcommon/vm_x86.cpp
	)

	set(QUAKE3_NASM_FILES
		unix/ftol.nasm
		unix/snapvector.nasm
	)

	foreach(SRC ${QUAKE3_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		set(QUAKE3_FILES ${QUAKE3_FILES} ${OBJ})
	endforeach(SRC)
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(QUAKE3_FILES ${QUAKE3_FILES}
		qcommon/vm_none.cpp
	)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlquake3 ${QUAKE3_FILES})
set_target_properties(jlquake3 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlquake3 botlib ${CLIENTS_LIBS})

#############################################################################
# DEDICATED SERVER                        
#############################################################################

set(QUAKE3_DEDICATED_FILES
	server/sv_bot.cpp
	server/sv_client.cpp
	server/sv_ccmds.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	server/sv_world.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	game/q_math.cpp
	game/q_shared.cpp
	qcommon/vm.cpp
	qcommon/vm_interpreted.cpp
	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	null/null_client.cpp
	null/null_snddma.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(QUAKE3_DEDICATED_FILES ${QUAKE3_DEDICATED_FILES}
		qcommon/vm_x86.cpp
	)

	set(QUAKE3_DED_NASM_FILES
		unix/ftol.nasm
		unix/snapvector.nasm
	)

	foreach(SRC ${QUAKE3_DED_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		set(QUAKE3_DEDICATED_FILES ${QUAKE3_DEDICATED_FILES} ${OBJ})
	endforeach(SRC)
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(QUAKE3_DEDICATED_FILES ${QUAKE3_DEDICATED_FILES}
		qcommon/vm_none.cpp
	)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlquake3ded ${QUAKE3_DEDICATED_FILES})
set_target_properties(jlquake3ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED -DC_ONLY")
target_link_libraries(jlquake3ded botlib ${SERVERS_LIBS})
