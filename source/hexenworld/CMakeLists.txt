add_executable(jlhwsv
	../apps/main.cpp
	../apps/unix.cpp
)
set_target_properties(jlhwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlhwsv ${SERVERS_LIBS})

add_executable(jlhwcl
	../apps/main.cpp
	../apps/unix.cpp
)
set_target_properties(jlhwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlhwcl ${CLIENTS_LIBS})
