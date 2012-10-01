# NOTE TTimo using -fshort-enums increases warnings on enum issues
#   this is for debugging and QA ONLY, the flag has ABI issues (OpenGL headers)
#$COMMON_CFLAGS = $COMMON_CFLAGS . '-fshort-enums ';

set(WOLFMP_FILES
	client/cl_cgame.cpp
	client/cl_input.cpp
	client/cl_keys.cpp
	client/cl_main.cpp
	client/cl_net_chan.cpp
	client/cl_parse.cpp
	client/cl_scrn.cpp

	qcommon/common.cpp
	qcommon/files.cpp

	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

add_executable(jlwolfmp ${WOLFMP_FILES})
set_target_properties(jlwolfmp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfmp ${CLIENTS_LIBS})

set(WOLFMPDED_FILES
	null/null_client.cpp
	qcommon/common.cpp
	qcommon/files.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

add_executable(jlwolfmpded ${WOLFMPDED_FILES})
set_target_properties(jlwolfmpded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O -DDEDICATED -DC_ONLY")
target_link_libraries(jlwolfmpded ${SERVERS_LIBS} m)
