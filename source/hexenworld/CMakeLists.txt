include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

set(BASE_CFLAGS "-Wall")
set(DEBUG_CFLAGS "${BASE_CFLAGS} -g")
set(RELEASE_CFLAGS "${BASE_CFLAGS} -m486 -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2
	-malign-jumps=2 -malign-functions=2")

set(SV_LDFLAGS -lm)
set(LDFLAGS -L/usr/X11/lib -lm -lGL -lX11 -lXext)

#############################################################################
# SERVER
#############################################################################

set(HWSV_FILES
	server/pr_cmds.cpp 
	server/pr_edict.cpp
	server/pr_exec.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_effect.cpp
	server/sv_ents.cpp
	server/sv_send.cpp
	server/sv_move.cpp
	server/sv_phys.cpp
	server/sv_user.cpp
	server/sv_ccmds.cpp
	server/world.cpp
	server/sys_unix.cpp
	server/model.cpp
	client/cmd.cpp
	client/common.cpp
	client/cvar.cpp
	client/strings.cpp
	client/zone.cpp
	client/pmove.cpp
	client/pmovetst.cpp
	client/net_chan.cpp
	client/net_udp.cpp
)

add_executable(vhwsv ${HWSV_FILES})
set_target_properties(vhwsv PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -DSERVERONLY")
target_link_libraries(vhwsv core ${SV_LDFLAGS})
add_dependencies(vhwsv core)

#############################################################################
# GL CLIENT
#############################################################################

set(HWCL_FILES
	client/cl_demo.cpp
	client/cl_effect.cpp
	client/cl_ents.cpp
	client/cl_input.cpp
	client/cl_main.cpp
	client/cl_parse.cpp
	client/cl_pred.cpp
	client/cl_tent.cpp
	client/cl_cam.cpp
	client/cmd.cpp
	client/common.cpp
	client/console.cpp
	client/cvar.cpp
	client/keys.cpp
	client/menu.cpp
	client/midi.cpp
	client/net_chan.cpp
	client/net_udp.cpp
	client/pmove.cpp
	client/pmovetst.cpp
	client/r_part.cpp
	client/sbar.cpp
	client/skin.cpp
	client/snd_dma.cpp
	client/snd_mem.cpp
	client/snd_mix.cpp
	client/strings.cpp
	client/view.cpp
	client/wad.cpp
	client/zone.cpp
	client/cd_linux.cpp
	client/sys_linux.cpp
	client/snd_linux.cpp
	client/gl_draw.cpp
	client/gl_mesh.cpp
	client/gl_model.cpp
	client/gl_ngraph.cpp
	client/gl_refrag.cpp
	client/gl_rlight.cpp
	client/gl_rmain.cpp
	client/gl_rmisc.cpp
	client/gl_rsurf.cpp
	client/gl_screen.cpp
	client/gl_vidlinuxglx.cpp
	client/gl_warp.cpp
)

set(HWCL_ASM_FILES
	client/snd_mixa.s
	client/sys_dosa.s
)

foreach(SRC ${HWCL_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/client/asm_i386.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(HWCL_FILES ${HWCL_FILES} ${OBJ})
endforeach(SRC)

add_executable(vhwcl ${HWCL_FILES})
set_target_properties(vhwcl PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(vhwcl core ${LDFLAGS})
add_dependencies(vhwcl core)
