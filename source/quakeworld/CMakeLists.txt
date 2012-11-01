include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

#############################################################################
# SERVER
#############################################################################

add_executable(jlqwsv
	../apps/unix.cpp
	server/sv_main.cpp
)
set_target_properties(jlqwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlqwsv ${SERVERS_LIBS})

#############################################################################
# CLIENT
#############################################################################

add_executable(jlqwcl
	../apps/unix.cpp
	client/cl_main.cpp
)
set_target_properties(jlqwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlqwcl ${CLIENTS_LIBS})
