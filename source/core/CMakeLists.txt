set(CORE_FILES
	clip_map/bsp29/load.cpp
	clip_map/bsp29/main.cpp
	clip_map/bsp29/test.cpp
	clip_map/bsp29/trace.cpp
	clip_map/bsp38/load.cpp
	clip_map/bsp38/main.cpp
	clip_map/bsp38/test.cpp
	clip_map/bsp38/trace.cpp
	clip_map/bsp46/load.cpp
	clip_map/bsp46/main.cpp
	clip_map/bsp46/patch.cpp
	clip_map/bsp46/polylib.cpp
	clip_map/bsp46/test.cpp
	clip_map/bsp46/trace.cpp
	clip_map/main.cpp
	clip_map/non_map.cpp
	virtual_machine/interpreted.cpp
	virtual_machine/main.cpp
	command_buffer.cpp
	Common.cpp
	common_defs.cpp
	console_variable.cpp
	crc.cpp
	endian.cpp
	event_queue.cpp
	exception.cpp
	files.cpp
	huffman.cpp
	info_string.cpp
	log.cpp
	mathlib.cpp
	md4.cpp
	memory.cpp
	message.cpp
	network_channel.cpp
	player_move.cpp
	player_move1.cpp
	socket.cpp
	socket_unix.cpp
	strings.cpp
	system_console_unix.cpp
	system_unix.cpp
	unzip.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(CORE_FILES ${CORE_FILES}
		virtual_machine/x86.cpp
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

	set(CORE_NASM_FILES
		ftol.nasm
	)

	foreach(SRC ${CORE_NASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC}
			COMMAND nasm -f elf -o ${OBJ} ${CMAKE_CURRENT_SOURCE_DIR}/${SRC}
		)
		set(CORE_FILES ${CORE_FILES} ${OBJ})
	endforeach(SRC)
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(CORE_FILES ${CORE_FILES}
		virtual_machine/none.cpp
	)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_library(core STATIC ${CORE_FILES})
set_target_properties(core PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(core zlib)
