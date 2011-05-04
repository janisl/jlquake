add_subdirectory(botlib)

set(THREAD_LDFLAGS -lpthread)
set(LDFLAGS -ldl -lm)
set(GLLDFLAGS ${LDFLAGS} -L/usr/X11R6/lib -lX11 -lXext -lXxf86dga -lXxf86vm -lasound -lGL)

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
	renderer/tr_animation.cpp
	renderer/tr_backend.cpp
	renderer/tr_bsp.cpp
	renderer/tr_cmds.cpp
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
	renderer/tr_shader.cpp
	renderer/tr_shade_calc.cpp
	renderer/tr_shadows.cpp
	renderer/tr_sky.cpp
	renderer/tr_surface.cpp
	renderer/tr_world.cpp
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
set_target_properties(jlquake3 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake3 botlib client core jpeg zlib ${GLLDFLAGS})
add_dependencies(jlquake3 botlib client jpeg)

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
target_link_libraries(jlquake3ded botlib core zlib ${LDFLAGS})
add_dependencies(jlquake3ded core botlib zlib)
