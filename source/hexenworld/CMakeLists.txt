include_directories(${CMAKE_CURRENT_SOURCE_DIR}/client)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/server)

#############################################################################
# SERVER
#############################################################################

add_executable(jlhwsv
	../apps/main.cpp
	../apps/unix.cpp
	server/sv_main.cpp
)
set_target_properties(jlhwsv PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlhwsv ${SERVERS_LIBS})

#############################################################################
# CLIENT
#############################################################################

add_executable(jlhwcl
	../apps/main.cpp
	../apps/unix.cpp
	client/cl_main.cpp
)
set_target_properties(jlhwcl PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
target_link_libraries(jlhwcl ${CLIENTS_LIBS})
