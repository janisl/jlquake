set(WOLFCLIENT_FILES
	../client/input/keys.cpp
	client_main.cpp
)

add_library(wolfclient STATIC ${WOLFCLIENT_FILES})
set_target_properties(wolfclient PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(wolfclient core)
