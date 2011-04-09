set(CORE_FILES
	cm_main.cpp
	cm_nonmap.cpp
	cm29_load.cpp
	cm29_main.cpp
	cm29_test.cpp
	cm29_trace.cpp
	cm38_load.cpp
	cm38_main.cpp
	cm38_test.cpp
	cm38_trace.cpp
	cm46_load.cpp
	cm46_main.cpp
	cm46_patch.cpp
	cm46_polylib.cpp
	cm46_test.cpp
	cm46_trace.cpp
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
