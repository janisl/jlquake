#  add_definitions(-DWOLF_SP_DEMO)
# NOTE TTimo using -fshort-enums increases warnings on enum issues
#   this is for debugging and QA ONLY, the flag has ABI issues (OpenGL headers)
#$COMMON_CFLAGS = $COMMON_CFLAGS . '-fshort-enums ';
#for release
#-malign-functions=2

set(WOLFSP_FILES
	qcommon/common.cpp
	unix/unix_main.cpp
)

add_executable(jlwolfsp ${WOLFSP_FILES})
set_target_properties(jlwolfsp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlwolfsp ${CLIENTS_LIBS})
