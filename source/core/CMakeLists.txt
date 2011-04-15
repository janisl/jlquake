set(CORE_FILES
	clipmap_main.cpp
	clipmap_nonmap.cpp
	clipmap29_load.cpp
	clipmap29_main.cpp
	clipmap29_test.cpp
	clipmap29_trace.cpp
	clipmap38_load.cpp
	clipmap38_main.cpp
	clipmap38_test.cpp
	clipmap38_trace.cpp
	clipmap46_load.cpp
	clipmap46_main.cpp
	clipmap46_patch.cpp
	clipmap46_polylib.cpp
	clipmap46_test.cpp
	clipmap46_trace.cpp
	cmd.cpp
	common.cpp
	crc.cpp
	cvar.cpp
	endian.cpp
	event_queue.cpp
	exception.cpp
	files.cpp
	huffman.cpp
	infostr.cpp
	log.cpp
	mathlib.cpp
	md4.cpp
	memory.cpp
	message.cpp
	socket.cpp
	socket_unix.cpp
	str.cpp
	system_unix.cpp
	unzip.cpp
)

set(CORE_ASM_FILES
	matha.s
)

foreach(SRC ${CORE_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/qasm.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(CORE_FILES ${CORE_FILES} ${OBJ})
endforeach(SRC)

add_library(core STATIC ${CORE_FILES})
add_dependencies(core zlib)
