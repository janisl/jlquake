add_executable(jlhexen2
	common.cpp
	host.cpp
	host_cmd.cpp
	sys_linux.cpp
)
set_target_properties(jlhexen2 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlhexen2 ${CLIENTS_LIBS})

add_executable(jlhexen2ded
	common.cpp
	host.cpp
	host_cmd.cpp
	sys_linux.cpp
)
set_target_properties(jlhexen2ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlhexen2ded ${SERVERS_LIBS})
