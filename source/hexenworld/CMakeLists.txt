include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

set(BASE_CFLAGS "-Wall -Dstricmp=strcasecmp -DGLQUAKE -DGL_EXT_SHARED")
set(DEBUG_CFLAGS "${BASE_CFLAGS} -g")
set(RELEASE_CFLAGS "${BASE_CFLAGS} -m486 -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2
	-malign-jumps=2 -malign-functions=2")

set(SV_LDFLAGS -lm)
set(LDFLAGS -L/usr/X11/lib -lm -lGL -lX11 -lXext)

#############################################################################
# SERVER
#############################################################################

set(QWSV_OBJS
	server/pr_cmds.c 
	server/pr_edict.c
	server/pr_exec.c
	server/sv_init.c
	server/sv_main.c
	server/sv_effect.c
	server/sv_ents.c
	server/sv_send.c
	server/sv_move.c
	server/sv_phys.c
	server/sv_user.c
	server/sv_ccmds.c
	server/world.c
	server/sys_unix.c
	server/model.c
	client/cmd.c
	client/common.c
	client/crc.c
	client/cvar.c
	client/mathlib.c
	client/strings.c
	client/zone.c
	client/pmove.c
	client/pmovetst.c
	client/net_chan.c
	client/net_udp.c
)

add_executable(vhwsv ${QWSV_OBJS})
set_target_properties(vhwsv PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -DSERVERONLY")
target_link_libraries(vhwsv ${SV_LDFLAGS})

#############################################################################
# GL CLIENT
#############################################################################

set(GLQWCL_OBJS
	client/cl_demo.c
	client/cl_effect.c
	client/cl_ents.c
	client/cl_input.c
	client/cl_main.c
	client/cl_parse.c
	client/cl_pred.c
	client/cl_tent.c
	client/cl_cam.c
	client/cmd.c
	client/common.c
	client/console.c
	client/crc.c
	client/cvar.c
	client/keys.c
	client/mathlib.c
	client/menu.c
	client/midi.c
	client/net_chan.c
	client/net_udp.c
	client/pmove.c
	client/pmovetst.c
	client/r_part.c
	client/sbar.c
	client/skin.c
	client/snd_dma.c
	client/snd_mem.c
	client/snd_mix.c
	client/strings.c
	client/view.c
	client/wad.c
	client/zone.c
	client/cd_linux.c
	client/sys_linux.c
	client/snd_linux.c
	client/gl_draw.c
	client/gl_mesh.c
	client/gl_model.c
	client/gl_ngraph.c
	client/gl_refrag.c
	client/gl_rlight.c
	client/gl_rmain.c
	client/gl_rmisc.c
	client/gl_rsurf.c
	client/gl_screen.c
	client/gl_vidlinuxglx.c
	client/gl_warp.c
)

set(QWCL_ASM_FILES
	client/math.s
	client/snd_mixa.s
	client/sys_dosa.s
)

foreach(SRC ${QWCL_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/client/asm_i386.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(GLQWCL_OBJS ${GLQWCL_OBJS} ${OBJ})
endforeach(SRC)

add_executable(vhwcl ${GLQWCL_OBJS})
set_target_properties(vhwcl PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(vhwcl ${LDFLAGS})
