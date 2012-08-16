#  add_definitions(-DWOLF_SP_DEMO)
# NOTE TTimo using -fshort-enums increases warnings on enum issues
#   this is for debugging and QA ONLY, the flag has ABI issues (OpenGL headers)
#$COMMON_CFLAGS = $COMMON_CFLAGS . '-fshort-enums ';
#for release
#-malign-functions=2

set(WOLFSP_FILES
	client/cl_cgame.cpp
	client/cl_cin.cpp
	client/cl_console.cpp
	client/cl_input.cpp
	client/cl_keys.cpp
	client/cl_main.cpp
	client/cl_net_chan.cpp
	client/cl_parse.cpp
	client/cl_scrn.cpp
	client/cl_ui.cpp

	qcommon/cmd.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp

	client/snd_dma.cpp

	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_glimp.cpp
)

add_executable(jlwolfsp ${WOLFSP_FILES})
set_target_properties(jlwolfsp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfsp ${CLIENTS_LIBS})
