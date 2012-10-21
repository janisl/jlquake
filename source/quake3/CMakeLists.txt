#############################################################################
# CLIENT/SERVER
#############################################################################

set(QUAKE3_FILES
	client/cl_main.cpp
	qcommon/common.cpp
	qcommon/files.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	unix/linux_signals.cpp
)

add_executable(jlquake3 ${QUAKE3_FILES})
set_target_properties(jlquake3 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlquake3 ${CLIENTS_LIBS})

#############################################################################
# DEDICATED SERVER                        
#############################################################################

set(QUAKE3_DEDICATED_FILES
	qcommon/common.cpp
	qcommon/files.cpp
	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
	null/null_client.cpp
)

add_executable(jlquake3ded ${QUAKE3_DEDICATED_FILES})
set_target_properties(jlquake3ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED -DC_ONLY")
target_link_libraries(jlquake3ded ${SERVERS_LIBS})
