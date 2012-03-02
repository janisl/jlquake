set(WOLFCORE_FILES
	../core/clip_map/bsp29/load.cpp
	../core/clip_map/bsp29/main.cpp
	../core/clip_map/bsp29/test.cpp
	../core/clip_map/bsp29/trace.cpp
	../core/clip_map/bsp38/load.cpp
	../core/clip_map/bsp38/main.cpp
	../core/clip_map/bsp38/test.cpp
	../core/clip_map/bsp38/trace.cpp
	../core/clip_map/bsp46/load.cpp
	../core/clip_map/bsp46/main.cpp
	../core/clip_map/bsp46/patch.cpp
	../core/clip_map/bsp46/polylib.cpp
	../core/clip_map/bsp46/test.cpp
	../core/clip_map/bsp46/trace.cpp
	../core/clip_map/main.cpp
	../core/clip_map/non_map.cpp
	../core/command_buffer.cpp
	../core/Common.cpp
	../core/common_defs.cpp
	../core/console_variable.cpp
	../core/crc.cpp
	../core/endian.cpp
	../core/event_queue.cpp
	../core/exception.cpp
	../core/files.cpp
	../core/huffman.cpp
	../core/info_string.cpp
	../core/log.cpp
	../core/mathlib.cpp
	../core/md4.cpp
	../core/memory.cpp
	../core/message.cpp
	socket.cpp
	../core/socket_unix.cpp
	../core/strings.cpp
	../core/system_console_unix.cpp
	../core/system_unix.cpp
	../core/unzip.cpp
)

if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(WOLFCORE_ASM_FILES
		../core/matha.s
	)

	foreach(SRC ${WOLFCORE_ASM_FILES})
		get_filename_component(FILE_BASE ${SRC} NAME_WE)
		set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
		add_custom_command(OUTPUT ${OBJ}
			DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/qasm.h
			COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
				-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
		)
		set(WOLFCORE_FILES ${WOLFCORE_FILES} ${OBJ})
	endforeach(SRC)
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

add_library(wolfcore STATIC ${WOLFCORE_FILES})
set_target_properties(wolfcore PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
