include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

#############################################################################
# SERVER
#############################################################################

add_executable(jlhwsv
	server/sv_main.cpp
	server/sv_send.cpp
	server/sv_ccmds.cpp
	server/sys_unix.cpp
	client/common.cpp
	client/net_chan.cpp
)
set_target_properties(jlhwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DSERVERONLY")
target_link_libraries(jlhwsv ${SERVERS_LIBS})

#############################################################################
# CLIENT
#############################################################################

add_executable(jlhwcl
	client/cl_demo.cpp
	client/cl_draw.cpp
	client/cl_input.cpp
	client/cl_main.cpp
	client/cl_parse.cpp
	client/cl_screen.cpp
	client/common.cpp
	client/console.cpp
	client/keys.cpp
	client/net_chan.cpp
	client/r_part.cpp
	client/snd_dma.cpp
	client/sys_linux.cpp
	client/view.cpp
)
set_target_properties(jlhwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlhwcl ${CLIENTS_LIBS})
