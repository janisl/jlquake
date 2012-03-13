set(WOLFCLIENT_FILES
	../client/input/keys.cpp
	../client/input/main.cpp
	../client/sound/audiocd/linux.cpp
	../client/sound/midi/null.cpp
	../client/sound/driver_alsa.cpp
	sound/main.cpp
	../client/sound/memory.cpp
	client_main.cpp
	../client/system_unix.cpp
)

add_library(wolfclient STATIC ${WOLFCLIENT_FILES})
set_target_properties(wolfclient PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(wolfclient core)
