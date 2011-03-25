set(CLIENT_FILES
	cd_linux.cpp
	render_driver_glx.cpp
	render_qgl.cpp
	snd_alsa.cpp
	snd_dma.cpp
	snd_mem.cpp
	snd_midi_null.cpp
	snd_mix.cpp
	#snd_oss.cpp
)

set(CLIENT_ASM_FILES
	snd_mixa.s
)

foreach(SRC ${CLIENT_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/qasm.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(CLIENT_FILES ${CLIENT_FILES} ${OBJ})
endforeach(SRC)

add_library(client STATIC ${CLIENT_FILES})
add_dependencies(client core)
