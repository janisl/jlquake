# NOTE TTimo using -fshort-enums increases warnings on enum issues
#   this is for debugging and QA ONLY, the flag has ABI issues (OpenGL headers)
#$COMMON_CFLAGS = $COMMON_CFLAGS . '-fshort-enums ';

set(WOLFMP_FILES
	../apps/main.cpp
	../apps/unix.cpp
	qcommon/common.cpp
)

add_executable(jlwolfmp ${WOLFMP_FILES})
set_target_properties(jlwolfmp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfmp ${CLIENTS_LIBS})

set(WOLFMPDED_FILES
	../apps/main.cpp
	../apps/unix.cpp
	qcommon/common.cpp
)

add_executable(jlwolfmpded ${WOLFMPDED_FILES})
set_target_properties(jlwolfmpded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O -DDEDICATED")
target_link_libraries(jlwolfmpded ${SERVERS_LIBS} m)
