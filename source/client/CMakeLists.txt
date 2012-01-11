set(CLIENT_FILES
	cinematic/cin.cpp
	cinematic/main.cpp
	cinematic/pcx.cpp
	cinematic/roq.cpp
	game/hexen2/chunk.cpp
	game/hexen2/effects.cpp
	game/hexen2/entities.cpp
	game/hexen2/explosion.cpp
	game/hexen2/light_effects.cpp
	game/hexen2/particle_effects.cpp
	game/hexen2/stream.cpp
	game/hexen2/temporary_entities.cpp
	game/quake/entities.cpp
	game/quake/light_effects.cpp
	game/quake/particle_effects.cpp
	game/quake/temporary_entities.cpp
	game/quake2/beam.cpp
	game/quake2/effects.cpp
	game/quake2/explosion.cpp
	game/quake2/laser.cpp
	game/quake2/light_effects.cpp
	game/quake2/main.cpp
	game/quake2/particle_effects.cpp
	game/quake2/sustain.cpp
	game/quake2/temporary_entities.cpp
	game/dynamic_lights.cpp
	game/light_styles.cpp
	game/particles.cpp
	input/keys.cpp
	input/main.cpp
	input/unix.cpp
	renderer/images/bmp.cpp
	renderer/images/image.cpp
	renderer/images/jpg.cpp
	renderer/images/pcx.cpp
	renderer/images/pic.cpp
	renderer/images/tga.cpp
	renderer/images/wal.cpp
	renderer/models/bsp29.cpp
	renderer/models/bsp38.cpp
	renderer/models/bsp46.cpp
	renderer/models/md2.cpp
	renderer/models/md3.cpp
	renderer/models/md4.cpp
	renderer/models/mdl.cpp
	renderer/models/model.cpp
	renderer/models/sp2.cpp
	renderer/models/spr.cpp
	renderer/backend.cpp
	renderer/commands.cpp
	renderer/curve.cpp
	renderer/driver_glx.cpp
	renderer/flares.cpp
	renderer/font.cpp
	renderer/init.cpp
	renderer/light.cpp
	renderer/main.cpp
	renderer/marks.cpp
	renderer/noise.cpp
	renderer/particle.cpp
	renderer/qgl.cpp
	renderer/scene.cpp
	renderer/screenshot.cpp
	renderer/shade.cpp
	renderer/shade_calculate.cpp
	renderer/shader.cpp
	renderer/shadows.cpp
	renderer/skin.cpp
	renderer/sky.cpp
	renderer/state.cpp
	renderer/surface1.cpp
	renderer/surface2.cpp
	renderer/surface3.cpp
	renderer/wad.cpp
	renderer/world.cpp
	sound/audiocd/linux.cpp
	sound/midi/null.cpp
	sound/driver_alsa.cpp
	#sound/driver_oss.cpp
	sound/main.cpp
	sound/memory.cpp
	sound/mix.cpp
	ui/console.cpp
	ui/draw.cpp
	ui/screen.cpp
	client_main.cpp
	system_unix.cpp
	utils.cpp
)

set(CLIENT_ASM_FILES
	sound/mix_x86.s
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
