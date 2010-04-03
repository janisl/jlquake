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
	botlib/be_aas_bspq3.cpp
	botlib/be_aas_cluster.cpp
	botlib/be_aas_debug.cpp
	botlib/be_aas_entity.cpp
	botlib/be_aas_file.cpp
	botlib/be_aas_main.cpp
	botlib/be_aas_move.cpp
	botlib/be_aas_optimize.cpp
	botlib/be_aas_reach.cpp
	botlib/be_aas_route.cpp
	botlib/be_aas_routealt.cpp
	botlib/be_aas_sample.cpp
	botlib/be_ai_char.cpp
	botlib/be_ai_chat.cpp
	botlib/be_ai_gen.cpp
	botlib/be_ai_goal.cpp
	botlib/be_ai_move.cpp
	botlib/be_ai_weap.cpp
	botlib/be_ai_weight.cpp
	botlib/be_ea.cpp
	botlib/be_interface.cpp
	botlib/l_crc.cpp
	botlib/l_libvar.cpp
	botlib/l_log.cpp
	botlib/l_memory.cpp
	botlib/l_precomp.cpp
	botlib/l_script.cpp
	botlib/l_struct.cpp
)

add_library(botlib STATIC ${BOTLIB_SOURCES})
set_target_properties(botlib PROPERTIES COMPILE_FLAGS "-DBOTLIB")
add_dependencies(botlib core)

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
	client/snd_adpcm.cpp
	client/snd_dma.cpp
	client/snd_mem.cpp
	client/snd_mix.cpp
	client/snd_wavelet.cpp
	server/sv_bot.cpp
	server/sv_ccmds.cpp
	server/sv_client.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_net_chan.cpp
	server/sv_snapshot.cpp
	server/sv_world.cpp
	qcommon/cm_load.cpp
	qcommon/cm_patch.cpp
	qcommon/cm_polylib.cpp
	qcommon/cm_test.cpp
	qcommon/cm_trace.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/md4.cpp
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	qcommon/huffman.cpp
	qcommon/unzip.cpp
	qcommon/vm.cpp
	qcommon/vm_interpreted.cpp
	qcommon/vm_x86.cpp
	game/q_shared.cpp
	game/q_math.cpp
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
	jpeg-6/jerror.cpp
	jpeg-6/jidctflt.c
	jpeg-6/jmemmgr.c
	jpeg-6/jmemnobs.cpp
	jpeg-6/jutils.c
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
	unix/unix_net.cpp
	unix/unix_shared.cpp
	unix/linux_common.cpp
	unix/linux_joystick.cpp
	unix/linux_glimp.cpp
	unix/linux_qgl.cpp
	unix/linux_signals.cpp
	unix/linux_snd.cpp
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
target_link_libraries(vquake3 botlib core ${GLLDFLAGS})
add_dependencies(vquake3 botlib core)

#add_executable(vquake3-smp ${QUAKE3_FILES})
#set_target_properties(vquake3-smp PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -DSMP")
#target_link_libraries(vquake3-smp botlib ${THREAD_LDFLAGS} ${GLLDFLAGS})
#add_dependencies(vquake3-smp botlib)

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
	qcommon/cm_load.cpp
	qcommon/cm_patch.cpp
	qcommon/cm_polylib.cpp
	qcommon/cm_test.cpp
	qcommon/cm_trace.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/md4.cpp
	qcommon/msg.cpp
	qcommon/net_chan.cpp
	qcommon/huffman.cpp
	game/q_math.cpp
	game/q_shared.cpp
	qcommon/unzip.cpp
	qcommon/vm.cpp
	qcommon/vm_interpreted.cpp
	unix/linux_common.cpp
	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_net.cpp
	unix/unix_shared.cpp
	null/null_client.cpp
	null/null_input.cpp
	null/null_snddma.cpp
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

add_executable(vquake3ded ${QUAKE3_DEDICATED_FILES})
set_target_properties(vquake3ded PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -DDEDICATED -DC_ONLY")
target_link_libraries(vquake3ded botlib core ${LDFLAGS})
add_dependencies(vquake3ded core botlib)
