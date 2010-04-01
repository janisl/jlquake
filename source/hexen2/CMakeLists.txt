set(RELEASE_CFLAGS -g -march=pentiumpro -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations)
set(DEBUG_CFLAGS -g)
set(LDFLAGS -L/usr/X11/lib -L/usr/local/lib -lGL -lX11 -lXext -ldl -lXxf86dga -lXxf86vm -lm)

set(GLQUAKE_OBJS
	cl_demo.c
	cl_effect.c
	cl_input.c
	cl_main.c
	cl_parse.c
	cl_tent.c
	chase.c
	cmd.c
	common.c
	console.c
	crc.c
	cvar.c
	gl_draw.c
	gl_mesh.c
	gl_model.c
	gl_refrag.c
	gl_rlight.c
	gl_rmain.c
	gl_rmisc.c
	gl_rsurf.c
	gl_screen.c
	gl_test.c
	gl_vidlinuxglx.c
	gl_warp.c
	host.c
	host_cmd.c
	keys.c
	menu.c
	mathlib.c
	midi.c
	net_dgrm.c
	net_loop.c
	net_main.c
	net_vcr.c
	net_udp.c
	net_bsd.c
	pr_cmds.c
	pr_edict.c
	pr_exec.c
	r_part.c
	sbar.c
	sv_main.c
	sv_phys.c
	sv_move.c
	sv_user.c
	zone.c
	view.c
	wad.c
	world.c
	cd_linux.c
	sys_linux.c
	snd_dma.c
	snd_mem.c
	snd_mix.c
	snd_linux.c
)

set(GLQUAKE_ASM_FILES
	math.s
	worlda.s
	snd_mixa.s
	sys_dosa.s
)

foreach(SRC ${GLQUAKE_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/asm_i386.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(GLQUAKE_OBJS ${GLQUAKE_OBJS} ${OBJ})
endforeach(SRC)

add_executable(vhexen2 ${GLQUAKE_OBJS})
set_target_properties(vhexen2 PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -Dstricmp=strcasecmp -DQUAKE2RJ -DRJNET -DGLQUAKE")
target_link_libraries(vhexen2 ${LDFLAGS})
