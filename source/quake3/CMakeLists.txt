set(BASE_CFLAGS "-pipe -fsigned-char -I/usr/X11R6/include")
set(DEBUG_CFLAGS "${BASE_CFLAGS} -g  -Wall -DNO_MOUSEGRAB -O")
set(RELEASE_CFLAGS "${BASE_CFLAGS} -DNDEBUG -O6 -mcpu=pentiumpro -march=pentium -fomit-frame-pointer -pipe -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce")

set(THREAD_LDFLAGS -lpthread)
set(LDFLAGS -ldl -lm)
set(GLLDFLAGS ${LDFLAGS} -L/usr/X11R6/lib -lX11 -lXext -lXxf86dga -lXxf86vm)

#############################################################################
# CLIENT/SERVER
#############################################################################

set(BOTLIB_SOURCES
	botlib/be_aas_bspq3.c
	botlib/be_aas_cluster.c
	botlib/be_aas_debug.c
	botlib/be_aas_entity.c
	botlib/be_aas_file.c
	botlib/be_aas_main.c
	botlib/be_aas_move.c
	botlib/be_aas_optimize.c
	botlib/be_aas_reach.c
	botlib/be_aas_route.c
	botlib/be_aas_routealt.c
	botlib/be_aas_sample.c
	botlib/be_ai_char.c
	botlib/be_ai_chat.c
	botlib/be_ai_gen.c
	botlib/be_ai_goal.c
	botlib/be_ai_move.c
	botlib/be_ai_weap.c
	botlib/be_ai_weight.c
	botlib/be_ea.c
	botlib/be_interface.c
	botlib/l_crc.c
	botlib/l_libvar.c
	botlib/l_log.c
	botlib/l_memory.c
	botlib/l_precomp.c
	botlib/l_script.c
	botlib/l_struct.c
)

add_library(botlib STATIC ${BOTLIB_SOURCES})
set_target_properties(botlib PROPERTIES COMPILE_FLAGS "-DBOTLIB")

set(QUAKE3_FILES
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
	client/snd_adpcm.c
	client/snd_dma.c
	client/snd_mem.c
	client/snd_mix.c
	client/snd_wavelet.c
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
	qcommon/md4.c
	qcommon/msg.c
	qcommon/net_chan.c
	qcommon/huffman.c
	qcommon/unzip.c
	qcommon/vm.c
	qcommon/vm_interpreted.c
	qcommon/vm_x86.c
	game/q_shared.c
	game/q_math.c
	jpeg-6/jcapimin.c
	jpeg-6/jchuff.c
	jpeg-6/jcinit.c
	jpeg-6/jccoefct.c
	jpeg-6/jccolor.c
	jpeg-6/jfdctflt.c
	jpeg-6/jcdctmgr.c
	jpeg-6/jcmainct.c
	jpeg-6/jcmarker.c
	jpeg-6/jcmaster.c
	jpeg-6/jcomapi.c
	jpeg-6/jcparam.c
	jpeg-6/jcprepct.c
	jpeg-6/jcsample.c
	jpeg-6/jdapimin.c
	jpeg-6/jdapistd.c
	jpeg-6/jdatasrc.c
	jpeg-6/jdcoefct.c
	jpeg-6/jdcolor.c
	jpeg-6/jcphuff.c
	jpeg-6/jddctmgr.c
	jpeg-6/jdhuff.c
	jpeg-6/jdinput.c
	jpeg-6/jdmainct.c
	jpeg-6/jdmarker.c
	jpeg-6/jdmaster.c
	jpeg-6/jdpostct.c
	jpeg-6/jdsample.c
	jpeg-6/jdtrans.c
	jpeg-6/jerror.c
	jpeg-6/jidctflt.c
	jpeg-6/jmemmgr.c
	jpeg-6/jmemnobs.c
	jpeg-6/jutils.c
	renderer/tr_animation.c
	renderer/tr_backend.c
	renderer/tr_bsp.c
	renderer/tr_cmds.c
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
	renderer/tr_shader.c
	renderer/tr_shade_calc.c
	renderer/tr_shadows.c
	renderer/tr_sky.c
	renderer/tr_surface.c
	renderer/tr_world.c
	unix/unix_main.c
	unix/unix_net.c
	unix/unix_shared.c
	unix/linux_common.c
	unix/linux_joystick.c
	unix/linux_glimp.c
	unix/linux_qgl.c
	unix/linux_signals.c
	unix/linux_snd.c
)

set(QUAKE3_ASM_FILES
	unix/snd_mixa.s
	unix/matha.s
)

foreach(SRC ${QUAKE3_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/unix/qasm.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(QUAKE3_FILES ${QUAKE3_FILES} ${OBJ})
endforeach(SRC)

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

add_executable(vquake3 ${QUAKE3_FILES})
set_target_properties(vquake3 PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(vquake3 botlib ${GLLDFLAGS})
add_dependencies(vquake3 botlib)

#add_executable(vquake3-smp ${QUAKE3_FILES})
#set_target_properties(vquake3-smp PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -DSMP")
#target_link_libraries(vquake3-smp botlib ${THREAD_LDFLAGS} ${GLLDFLAGS})
#add_dependencies(vquake3-smp botlib)

#############################################################################
# DEDICATED SERVER                        
#############################################################################

set(QUAKE3_DEDICATED_FILES
	server/sv_bot.c
	server/sv_client.c
	server/sv_ccmds.c
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
	qcommon/md4.c
	qcommon/msg.c
	qcommon/net_chan.c
	qcommon/huffman.c
	game/q_math.c
	game/q_shared.c
	qcommon/unzip.c
	qcommon/vm.c
	qcommon/vm_interpreted.c
	unix/linux_common.c
	unix/linux_signals.c
	unix/unix_main.c
	unix/unix_net.c
	unix/unix_shared.c
	null/null_client.c
	null/null_input.c
	null/null_snddma.c
	qcommon/vm_x86.c
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

add_executable(vquake3ded ${QUAKE3_DEDICATED_FILES})
set_target_properties(vquake3ded PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -DDEDICATED -DC_ONLY")
target_link_libraries(vquake3ded botlib ${LDFLAGS})
add_dependencies(vquake3ded botlib)
