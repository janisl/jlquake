set(RELEASE_CFLAGS "-g -march=pentiumpro -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations")
set(DEBUG_CFLAGS "-g")
set(LDFLAGS -L/usr/X11/lib -L/usr/local/lib -lGL -lX11 -lXext -ldl -lXxf86dga -lXxf86vm -lm)

set(QUAKE_FILES
	cl_demo.cpp
	cl_input.cpp
	cl_main.cpp
	cl_parse.cpp
	cl_tent.cpp
	chase.cpp
	cmd.cpp
	common.cpp
	console.cpp
	crc.cpp
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
	net_dgrm.cpp
	net_loop.cpp
	net_main.cpp
	net_vcr.cpp
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
	view.cpp
	wad.cpp
	world.cpp
	zone.cpp
	cd_linux.cpp
	sys_linux.cpp
	snd_dma.cpp
	snd_mem.cpp
	snd_mix.cpp
	snd_linux.cpp
)

set(QUAKE_ASM_FILES
	worlda.s
	snd_mixa.s
	sys_dosa.s
)

foreach(SRC ${QUAKE_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/asm_i386.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(QUAKE_FILES ${QUAKE_FILES} ${OBJ})
endforeach(SRC)

add_executable(vquake ${QUAKE_FILES})
set_target_properties(vquake PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(vquake core ${LDFLAGS})
add_dependencies(vquake core)
