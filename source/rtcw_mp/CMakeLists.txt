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
	renderer/tr_animation.c   
	renderer/tr_backend.c   
	renderer/tr_bsp.c   
	renderer/tr_cmds.c   
	renderer/tr_cmesh.c
	renderer/tr_curve.c   
	renderer/tr_flares.c   
	renderer/tr_font.c    
	renderer/tr_image.c    
	renderer/tr_init.c     
	renderer/tr_light.c   
	renderer/tr_main.c    
	renderer/tr_marks.c    
	renderer/tr_mesh.c    
	renderer/tr_model.c    
	renderer/tr_noise.c    
	renderer/tr_scene.c    
	renderer/tr_shade.c    
	renderer/tr_shade_calc.c   
	renderer/tr_shader.c    
	renderer/tr_shadows.c    
	renderer/tr_sky.c    
	renderer/tr_surface.c   
	renderer/tr_world.c    

	client/cl_cgame.c   
	client/cl_cin.c       
	client/cl_console.c  
	client/cl_input.c   
	client/cl_keys.c     
	client/cl_main.c     
	client/cl_net_chan.c  
	client/cl_parse.c   
	client/cl_scrn.c    
	client/cl_ui.c         

	qcommon/cm_load.c        
	qcommon/cm_patch.c       
	qcommon/cm_polylib.c     
	qcommon/cm_test.c        
	qcommon/cm_trace.c       
	qcommon/cmd.c                   
	qcommon/common.c               
	qcommon/cvar.c                
	qcommon/files.c              
	qcommon/md4.c                   
	qcommon/msg.c                    
	qcommon/net_chan.c
	qcommon/huffman.c
	qcommon/unzip.c
	qcommon/vm.c
	qcommon/vm_interpreted.c

	client/snd_adpcm.c
	client/snd_dma.c
	client/snd_mem.c
	client/snd_mix.c
	client/snd_wavelet.c

	unix/unix_main.c 
	unix/unix_net.c 
	unix/unix_shared.c 
	unix/linux_common.c
	unix/linux_qgl.c   
	unix/linux_glimp.c   
	unix/linux_joystick.c  
	unix/linux_snd.c
	unix/linux_signals.c

	server/sv_bot.c
	server/sv_ccmds.c
	server/sv_client.c
	server/sv_game.c
	server/sv_init.c
	server/sv_main.c
	server/sv_net_chan.c
	server/sv_snapshot.c
	server/sv_world.c

	game/q_math.c
	game/q_shared.c
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	list(APPEND WOLFMP_FILES
		qcommon/vm_x86.c
	)

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

	set(WOLFMP_ASM_FILES
		unix/matha.s
		unix/snd_mixa.s
	)
	foreach(SRC ${WOLFMP_ASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/qasm.h
			COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
				-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
		)
		list(APPEND WOLFMP_FILES ${OBJ})
	endforeach(SRC)
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
	list(APPEND WOLFMP_FILES
		qcommon/vm_none.c
	)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlwolfmp ${WOLFMP_FILES})
set_target_properties(jlwolfmp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfmp botlib-wolfmp splines-wolfmp ${WOLF_CLIENTS_LIBS})

set(WOLFMPDED_FILES
	null/null_client.c
	null/null_input.c
	null/null_snddma.c
	server/sv_bot.c
	server/sv_ccmds.c
	server/sv_client.c
	server/sv_game.c
	server/sv_init.c
	server/sv_main.c
	server/sv_net_chan.c
	server/sv_snapshot.c
	server/sv_world.c
	qcommon/cm_load.c
	qcommon/cm_patch.c
	qcommon/cm_polylib.c
	qcommon/cm_test.c
	qcommon/cm_trace.c
	qcommon/cmd.c
	qcommon/common.c
	qcommon/cvar.c
	qcommon/files.c
	qcommon/huffman.c
	qcommon/md4.c
	qcommon/msg.c
	qcommon/net_chan.c
	qcommon/unzip.c
	qcommon/vm.c
	qcommon/vm_interpreted.c
	game/q_math.c
	game/q_shared.c
	unix/linux_common.c
	unix/unix_main.c
	unix/unix_net.c
	unix/unix_shared.c
	unix/linux_signals.c
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	list(APPEND WOLFMPDED_FILES
		qcommon/vm_x86.c
	)

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
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
	list(APPEND WOLFMPDED_FILES
		qcommon/vm_none.c
	)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_executable(jlwolfmpded ${WOLFMPDED_FILES})
set_target_properties(jlwolfmpded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O -DDEDICATED -DC_ONLY")
target_link_libraries(jlwolfmpded botlib-wolfmp ${WOLF_SERVERS_LIBS} m)
