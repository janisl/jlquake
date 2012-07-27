include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

#############################################################################
# SERVER
#############################################################################

add_executable(jlqwsv
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_ents.cpp
	server/sv_send.cpp
	server/sv_user.cpp      
	server/sv_ccmds.cpp
	server/sys_unix.cpp
	client/cmd.cpp
	client/common.cpp
	client/cvar.cpp
	client/zone.cpp
	client/net_chan.cpp
)
set_target_properties(jlqwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DSERVERONLY")
target_link_libraries(jlqwsv ${SERVERS_LIBS})

#############################################################################
# CLIENT
#############################################################################

add_executable(jlqwcl
	client/cl_demo.cpp
	client/cl_draw.cpp
	client/cl_input.cpp
	client/cl_main.cpp
	client/cl_parse.cpp
	client/cl_screen.cpp
	client/cmd.cpp
	client/common.cpp
	client/console.cpp
	client/cvar.cpp
	client/keys.cpp
	client/menu.cpp
	client/net_chan.cpp
	client/sbar.cpp
	client/snd_dma.cpp
	client/sys_linux.cpp
	client/view.cpp
	client/zone.cpp
)
set_target_properties(jlqwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlqwcl ${DEDCLIENTS_LIBS})
