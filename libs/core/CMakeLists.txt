set(CORE_FILES
	array.h
	cmd.cpp
	cmd.h
	common.cpp
	common.h
	core.h
	crc.cpp
	crc.h
	cvar.cpp
	cvar.h
	endian.cpp
	endian.h
	exception.cpp
	exception.h
	huffman.cpp
	huffman.h
	log.cpp
	log.h
	mathlib.cpp
	mathlib.h
	md4.cpp
	md4.h
	memory.cpp
	memory.h
	message.cpp
	message.h
	str.cpp
	str.h
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
