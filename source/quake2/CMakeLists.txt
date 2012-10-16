add_executable(jlquake2
	client/cl_main.cpp
	client/cl_pred.cpp
	qcommon/common.cpp
	qcommon/files.cpp
	qcommon/q_shared.cpp
	linux/sys_linux.cpp
)
set_target_properties(jlquake2 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake2 ${CLIENTS_LIBS})

add_executable(jlquake2ded
	qcommon/common.cpp
	qcommon/files.cpp
	qcommon/q_shared.cpp
	linux/sys_linux.cpp
	null/cl_null.cpp
)
set_target_properties(jlquake2ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED_ONLY")
target_link_libraries(jlquake2ded ${SERVERS_LIBS})
