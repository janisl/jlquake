add_executable(jlquake
	../apps/unix.cpp
	common.cpp
	host.cpp
)
set_target_properties(jlquake PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlquake ${CLIENTS_LIBS})

add_executable(jlquakeded
	../apps/unix.cpp
	common.cpp
	host.cpp
)
set_target_properties(jlquakeded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlquakeded ${SERVERS_LIBS})
