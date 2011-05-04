set(GLLDFLAGS -L/usr/X11R6/lib -lGL -lX11 -lXext -lXxf86dga -lXxf86vm -lasound)

add_executable(jlquake2
	client/cl_cin.cpp
	client/cl_ents.cpp
	client/cl_fx.cpp
	client/cl_input.cpp
	client/cl_inv.cpp
	client/cl_main.cpp
	client/cl_newfx.cpp
	client/cl_parse.cpp
	client/cl_pred.cpp
	client/cl_tent.cpp
	client/cl_scrn.cpp
	client/cl_view.cpp
	client/console.cpp
	client/keys.cpp
	client/menu.cpp
	client/snd_dma.cpp
	client/qmenu.cpp
	qcommon/m_flash.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp
	qcommon/q_shared.cpp
	qcommon/pmove.cpp
	server/sv_ccmds.cpp
	server/sv_ents.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_send.cpp
	server/sv_user.cpp
	server/sv_world.cpp
	linux/q_shlinux.cpp
	linux/sys_linux.cpp
	linux/glob.cpp
	ref_gl/gl_draw.cpp
	ref_gl/gl_image.cpp
	ref_gl/gl_light.cpp
	ref_gl/gl_mesh.cpp
	ref_gl/gl_model.cpp
	ref_gl/gl_rmain.cpp
	ref_gl/gl_rmisc.cpp
	ref_gl/gl_rsurf.cpp
	ref_gl/gl_warp.cpp
)
set_target_properties(jlquake2 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake2 client core zlib jpeg ${GLLDFLAGS})

add_executable(jlquake2ded
	qcommon/m_flash.cpp
	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp
	qcommon/q_shared.cpp
	qcommon/pmove.cpp
	server/sv_ccmds.cpp
	server/sv_ents.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_send.cpp
	server/sv_user.cpp
	server/sv_world.cpp
	linux/q_shlinux.cpp
	linux/sys_linux.cpp
	linux/glob.cpp
	null/cl_null.cpp
)
set_target_properties(jlquake2ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED_ONLY")
target_link_libraries(jlquake2ded core zlib dl)
