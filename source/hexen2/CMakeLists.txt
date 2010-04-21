set(RELEASE_CFLAGS "-g -march=pentiumpro -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations")
set(DEBUG_CFLAGS "-g")
set(LDFLAGS -L/usr/X11/lib -L/usr/local/lib -lGL -lX11 -lXext -ldl -lXxf86dga -lXxf86vm -lm -lasound)

set(HEXEN2_FILES
	cl_demo.cpp
	cl_effect.cpp
	cl_input.cpp
	cl_main.cpp
	cl_parse.cpp
	cl_tent.cpp
	chase.cpp
	cmd.cpp
	common.cpp
	console.cpp
	cvar.cpp
	gl_draw.cpp
	gl_mesh.cpp
	gl_model.cpp
	gl_refrag.cpp
	gl_rlight.cpp
	gl_rmain.cpp
	gl_rmisc.cpp
	gl_rsurf.cpp
	gl_screen.cpp
	gl_test.cpp
	gl_vidlinuxglx.cpp
	gl_warp.cpp
	host.cpp
	host_cmd.cpp
	keys.cpp
	menu.cpp
	midi.cpp
	net_dgrm.cpp
	net_loop.cpp
	net_main.cpp
	net_udp.cpp
	net_bsd.cpp
	pr_cmds.cpp
	pr_edict.cpp
	pr_exec.cpp
	r_part.cpp
	sbar.cpp
	sv_main.cpp
	sv_phys.cpp
	sv_move.cpp
	sv_user.cpp
	zone.cpp
	view.cpp
	wad.cpp
	world.cpp
	cd_linux.cpp
	sys_linux.cpp
	snd_dma.cpp
	snd_mem.cpp
	snd_mix.cpp
#	snd_linux.cpp
	snd_alsa.cpp
)

set(HEXEN2_ASM_FILES
	worlda.s
	snd_mixa.s
)

foreach(SRC ${HEXEN2_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/asm_i386.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(HEXEN2_FILES ${HEXEN2_FILES} ${OBJ})
endforeach(SRC)

add_executable(vhexen2 ${HEXEN2_FILES})
set_target_properties(vhexen2 PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(vhexen2 core zlib ${LDFLAGS})
add_dependencies(vhexen2 core)
