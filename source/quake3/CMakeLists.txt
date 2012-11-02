#############################################################################
# CLIENT/SERVER
#############################################################################

set(QUAKE3_FILES
	../apps/main.cpp
	../apps/unix.cpp
	qcommon/common.cpp
)

add_executable(jlquake3 ${QUAKE3_FILES})
set_target_properties(jlquake3 PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlquake3 ${CLIENTS_LIBS})

#############################################################################
# DEDICATED SERVER                        
#############################################################################

set(QUAKE3_DEDICATED_FILES
	../apps/main.cpp
	../apps/unix.cpp
	qcommon/common.cpp
)

add_executable(jlquake3ded ${QUAKE3_DEDICATED_FILES})
set_target_properties(jlquake3ded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -DDEDICATED")
target_link_libraries(jlquake3ded ${SERVERS_LIBS})
