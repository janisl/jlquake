set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wc++-compat")
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

add_subdirectory(botlib)

# splines
add_library(splines-wolfmp STATIC
	splines/math_angles.cpp
	splines/math_matrix.cpp
	splines/math_quaternion.cpp
	splines/math_vector.cpp
	splines/q_parse.cpp
	splines/splines.cpp
	splines/util_str.cpp
)

set(WOLFMP_FILES
	renderer/tr_animation.cpp
	renderer/tr_backend.cpp
	renderer/tr_bsp.cpp
	renderer/tr_cmds.cpp
	renderer/tr_cmesh.cpp
	renderer/tr_curve.cpp
	renderer/tr_flares.cpp
	renderer/tr_font.cpp
	renderer/tr_image.cpp
	renderer/tr_init.cpp
	renderer/tr_light.cpp
	renderer/tr_main.cpp
	renderer/tr_marks.cpp
	renderer/tr_mesh.cpp
	renderer/tr_model.cpp
	renderer/tr_noise.cpp
	renderer/tr_scene.cpp
	renderer/tr_shade.cpp
	renderer/tr_shade_calc.cpp
	renderer/tr_shader.cpp
	renderer/tr_shadows.cpp
	renderer/tr_sky.cpp
	renderer/tr_surface.cpp
	renderer/tr_world.cpp

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
	unix/linux_signals.cpp

	server/sv_bot.cpp
	server/sv_ccmds.cpp
	server/sv_client.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	server/sv_world.cpp

	game/q_math.cpp
	game/q_shared.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(WOLFMP_NASM_FILES
		unix/ftol.nasm
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
target_link_libraries(jlwolfmp botlib-wolfmp splines-wolfmp ${WOLF_CLIENTS_LIBS})

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
	server/sv_world.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	qcommon/vm.cpp
	game/q_math.cpp
	game/q_shared.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(WOLFMPDED_NASM_FILES
		unix/ftol.nasm
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
target_link_libraries(jlwolfmpded botlib-wolfmp ${SERVERS_LIBS} m)
