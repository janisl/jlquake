add_executable(jlqwsv
	../apps/main.cpp
	../apps/unix.cpp
	server/sv_main.cpp
)
set_target_properties(jlqwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlqwsv ${SERVERS_LIBS})

add_executable(jlqwcl
	../apps/main.cpp
	../apps/unix.cpp
	client/cl_main.cpp
)
set_target_properties(jlqwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlqwcl ${CLIENTS_LIBS})
