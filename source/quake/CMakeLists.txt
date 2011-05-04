set(LDFLAGS -L/usr/X11/lib -L/usr/local/lib -lGL -lX11 -lXext -ldl -lXxf86dga -lXxf86vm -lm -lasound)

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
	gl_warp.cpp
	host.cpp
	host_cmd.cpp
	keys.cpp
	menu.cpp
	net_dgrm.cpp
	net_loop.cpp
	net_main.cpp
	pr_cmds.cpp
	pr_edict.cpp
	pr_exec.cpp
	r_part.cpp
	sbar.cpp
	snd_dma.cpp
	sv_main.cpp
	sv_phys.cpp
	sv_move.cpp
	sv_user.cpp
	sys_linux.cpp
	view.cpp
	wad.cpp
	world.cpp
	zone.cpp
)

add_executable(jlquake ${QUAKE_FILES})
set_target_properties(jlquake PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake client progsvm core zlib jpeg ${LDFLAGS})
