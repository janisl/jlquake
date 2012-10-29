add_executable(jlquake2
	../apps/unix.cpp
	qcommon/common.cpp
	qcommon/q_shared.cpp
)
set_target_properties(jlquake2 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake2 ${CLIENTS_LIBS})

add_executable(jlquake2ded
	../apps/unix.cpp
	qcommon/common.cpp
	qcommon/q_shared.cpp
)
set_target_properties(jlquake2ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlquake2ded ${SERVERS_LIBS})
