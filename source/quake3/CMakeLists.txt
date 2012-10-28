#############################################################################
# CLIENT/SERVER
#############################################################################

set(QUAKE3_FILES
	qcommon/common.cpp
	unix/unix_main.cpp
)

add_executable(jlquake3 ${QUAKE3_FILES})
set_target_properties(jlquake3 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlquake3 ${CLIENTS_LIBS})

#############################################################################
# DEDICATED SERVER                        
#############################################################################

set(QUAKE3_DEDICATED_FILES
	qcommon/common.cpp
	unix/unix_main.cpp
)

add_executable(jlquake3ded ${QUAKE3_DEDICATED_FILES})
set_target_properties(jlquake3ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED -DC_ONLY")
target_link_libraries(jlquake3ded ${SERVERS_LIBS})
