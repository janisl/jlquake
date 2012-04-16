# don't wrap gcc messages
#BASECPPFLAGS.append( '-fmessage-length=0' )

#	BASECPPFLAGS.append( '-D_DEBUG' )
		# -fomit-frame-pointer: gcc manual indicates -O sets this implicitely,
		# only if that doesn't affect debugging
		# on Linux, this affects backtrace capability, so I'm assuming this is needed
		# -finline-functions: implicit at -O3
		# -fschedule-insns2: implicit at -O3
		# -funroll-loops ?
		# -mfpmath=sse -msse ?
		#OPTCPPFLAGS = [ '-O3', '-march=i686', '-Winline', '-ffast-math', '-fomit-frame-pointer', '-finline-functions', '-fschedule-insns2' ]

# maintain this dangerous optimization off at all times
#g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

add_subdirectory(botlib)

add_library(splines-et STATIC
	splines/math_angles.cpp
	splines/math_matrix.cpp
	splines/math_quaternion.cpp
	splines/math_vector.cpp
	splines/q_parse.cpp
	splines/splines.cpp
	splines/util_str.cpp
)

set (ET_FILES
	renderer/tr_font.cpp
	renderer/tr_init.cpp
	renderer/tr_main.cpp
	renderer/tr_marks.cpp
	renderer/tr_model.cpp
	renderer/tr_world.cpp

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
	qcommon/dl_main_curl.cpp

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

	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	game/q_shared.cpp
	game/q_math.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(ET_NASM_FILES
		unix/snapvector.nasm
	)
	foreach(SRC ${ET_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		list(APPEND ET_FILES ${OBJ})
	endforeach(SRC)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlet ${ET_FILES})
set_target_properties(jlet PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlet botlib-et splines-et ${WOLF_CLIENTS_LIBS} curl)

#local_env.Append( CPPDEFINES = [ 'MASTER_SERVER_NAME=\\"%s\\"' % MASTER ] )

set (ETDED_FILES
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

	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	game/q_shared.cpp
	game/q_math.cpp

	null/null_client.cpp
	null/null_snddma.cpp
	qcommon/dl_main_stubs.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(ETDED_NASM_FILES
		unix/snapvector.nasm
	)
	foreach(SRC ${ETDED_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		list(APPEND ETDED_FILES ${OBJ})
	endforeach(SRC)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jletded ${ETDED_FILES})
set_target_properties(jletded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O -DDEDICATED")
target_link_libraries(jletded botlib-et ${SERVERS_LIBS} m)
