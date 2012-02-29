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

	qcommon/cm_load.cpp
	qcommon/cm_trace.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	qcommon/huffman.cpp
	qcommon/vm.cpp
	qcommon/vm_interpreted.cpp

	client/snd_adpcm.cpp
	client/snd_dma.cpp
	client/snd_mem.cpp
	client/snd_mix.cpp
	client/snd_wavelet.cpp

	unix/unix_main.cpp
	unix/unix_net.cpp
	unix/unix_shared.cpp
	unix/linux_qgl.cpp
	unix/linux_glimp.cpp
	unix/linux_joystick.cpp
	unix/linux_snd.cpp

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
	list(APPEND WOLFSP_FILES
		qcommon/vm_x86.cpp
	)

	set(WOLFSP_NASM_FILES
		unix/ftol.nasm
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

	set(WOLFSP_ASM_FILES
		unix/snd_mixa.s
	)
	foreach(SRC ${WOLFSP_ASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/qasm.h
			COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
				-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
		)
		list(APPEND WOLFSP_FILES ${OBJ})
	endforeach(SRC)
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
	list(APPEND WOLFSP_FILES
		qcommon/vm_none.cpp
	)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlwolfsp ${WOLFSP_FILES})
set_target_properties(jlwolfsp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfsp botlib-wolfsp splines-wolfsp ${WOLF_CLIENTS_LIBS})
