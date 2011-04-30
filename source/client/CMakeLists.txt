set(CLIENT_FILES
	input_keys.cpp
	input_main.cpp
	input_unix.cpp
	render_driver_glx.cpp
	render_image.cpp
	render_image_bmp.cpp
	render_image_jpg.cpp
	render_image_pcx.cpp
	render_image_pic.cpp
	render_image_tga.cpp
	render_image_wal.cpp
	render_init.cpp
	render_qgl.cpp
	render_state.cpp
	sound_audiocd_linux.cpp
	sound_driver_alsa.cpp
	sound_main.cpp
	sound_memory.cpp
	sound_midi_null.cpp
	sound_mix.cpp
	#sound_driver_oss.cpp
)

set(CLIENT_ASM_FILES
	sound_mix_x86.s
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
add_dependencies(client core jpeg)
