set(CLIENT_FILES
	cinematic_cin.cpp
	cinematic_main.cpp
	cinematic_pcx.cpp
	cinematic_roq.cpp
	client_main.cpp
	input_keys.cpp
	input_main.cpp
	input_unix.cpp
	render_backend.cpp
	render_curve.cpp
	render_driver_glx.cpp
	render_font.cpp
	render_image.cpp
	render_image_bmp.cpp
	render_image_jpg.cpp
	render_image_pcx.cpp
	render_image_pic.cpp
	render_image_tga.cpp
	render_image_wal.cpp
	render_init.cpp
	render_light.cpp
	render_main.cpp
	render_model.cpp
	render_model_bsp29.cpp
	render_model_bsp38.cpp
	render_model_bsp46.cpp
	render_model_md2.cpp
	render_model_md3.cpp
	render_model_md4.cpp
	render_model_mdl.cpp
	render_model_sp2.cpp
	render_model_spr.cpp
	render_noise.cpp
	render_qgl.cpp
	render_scene.cpp
	render_shade.cpp
	render_shade_calculate.cpp
	render_shader.cpp
	render_shadows.cpp
	render_skin.cpp
	render_sky.cpp
	render_state.cpp
	render_surface1.cpp
	render_surface2.cpp
	render_surface3.cpp
	render_wad.cpp
	render_world.cpp
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
set_target_properties(client PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(client core jpeg)
