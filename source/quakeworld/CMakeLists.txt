include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

#############################################################################
# SERVER
#############################################################################

add_executable(jlqwsv
	server/pr_cmds.cpp 
	server/pr_edict.cpp
	server/pr_exec.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_nchan.cpp
	server/sv_ents.cpp
	server/sv_send.cpp
	server/sv_move.cpp
	server/sv_phys.cpp
	server/sv_user.cpp      
	server/sv_ccmds.cpp
	server/world.cpp
	server/sys_unix.cpp
	client/cmd.cpp
	client/common.cpp
	client/cvar.cpp
	client/zone.cpp
	client/pmove.cpp
	client/pmovetst.cpp
	client/net_chan.cpp
)
set_target_properties(jlqwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DSERVERONLY")
target_link_libraries(jlqwsv progsvm ${SERVERS_LIBS})

#############################################################################
# CLIENT
#############################################################################

add_executable(jlqwcl
	client/cl_demo.cpp
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
	client/net_chan.cpp
	client/pmove.cpp
	client/pmovetst.cpp
	client/r_part.cpp
	client/sbar.cpp
	client/skin.cpp
	client/snd_dma.cpp
	client/view.cpp
	client/zone.cpp
	client/sys_linux.cpp
	client/gl_draw.cpp
	client/gl_mesh.cpp
	client/gl_model.cpp
	client/gl_ngraph.cpp
	client/gl_rlight.cpp
	client/gl_rmain.cpp
	client/gl_rmisc.cpp
	client/gl_rsurf.cpp
	client/gl_screen.cpp
	client/gl_warp.cpp
)
set_target_properties(jlqwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlqwcl ${CLIENTS_LIBS})
