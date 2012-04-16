#  add_definitions(-DWOLF_SP_DEMO)
# NOTE TTimo using -fshort-enums increases warnings on enum issues
#   this is for debugging and QA ONLY, the flag has ABI issues (OpenGL headers)
#$COMMON_CFLAGS = $COMMON_CFLAGS . '-fshort-enums ';
#for release
#-malign-functions=2

add_subdirectory(botlib)

# splines
add_library(splines-wolfsp STATIC
	splines/math_angles.cpp
	splines/math_matrix.cpp
	splines/math_quaternion.cpp
	splines/math_vector.cpp
	splines/q_parse.cpp
	splines/splines.cpp
	splines/util_str.cpp
)


set(WOLFSP_FILES
	renderer/tr_init.cpp
	renderer/tr_main.cpp
	renderer/tr_marks.cpp
	renderer/tr_model.cpp

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
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	qcommon/vm.cpp

	client/snd_dma.cpp

	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_glimp.cpp

	server/sv_bot.cpp
	server/sv_ccmds.cpp
	server/sv_client.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	server/sv_world.cpp

	game/q_shared.cpp
	game/q_math.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(WOLFSP_NASM_FILES
		unix/snapvector.nasm
	)
	foreach(SRC ${WOLFSP_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		list(APPEND WOLFSP_FILES ${OBJ})
	endforeach(SRC)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlwolfsp ${WOLFSP_FILES})
set_target_properties(jlwolfsp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfsp botlib-wolfsp splines-wolfsp ${WOLF_CLIENTS_LIBS})
