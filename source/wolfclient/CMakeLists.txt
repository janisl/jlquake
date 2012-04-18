set(WOLFCLIENT_FILES
	../client/cinematic/cin.cpp
	../client/cinematic/main.cpp
	../client/cinematic/pcx.cpp
	../client/cinematic/roq.cpp
	../client/game/hexen2/chunk.cpp
	../client/game/hexen2/effects.cpp
	../client/game/hexen2/entities.cpp
	../client/game/hexen2/explosion.cpp
	../client/game/hexen2/light_effects.cpp
	../client/game/hexen2/particle_effects.cpp
	../client/game/hexen2/projectiles.cpp
	../client/game/hexen2/stream.cpp
	../client/game/hexen2/temporary_entities.cpp
	../client/game/quake2/beam.cpp
	../client/game/quake2/effects.cpp
	../client/game/quake2/explosion.cpp
	../client/game/quake2/laser.cpp
	../client/game/quake2/light_effects.cpp
	../client/game/quake2/main.cpp
	../client/game/quake2/particle_effects.cpp
	../client/game/quake2/sustain.cpp
	../client/game/quake2/temporary_entities.cpp
	../client/game/dynamic_lights.cpp
	../client/game/light_styles.cpp
	../client/game/particles.cpp
	../client/input/keys.cpp
	../client/input/main.cpp
	../client/input/unix.cpp
	../client/renderer/images/bmp.cpp
	../client/renderer/images/image.cpp
	../client/renderer/images/jpg.cpp
	../client/renderer/images/pcx.cpp
	../client/renderer/images/pic.cpp
	../client/renderer/images/tga.cpp
	../client/renderer/images/wal.cpp
	../client/renderer/models/bsp29.cpp
	../client/renderer/models/bsp38.cpp
	../client/renderer/models/bsp46.cpp
	../client/renderer/models/md2.cpp
	../client/renderer/models/md3.cpp
	../client/renderer/models/md4.cpp
	../client/renderer/models/mdc.cpp
	../client/renderer/models/mdl.cpp
	../client/renderer/models/mdm.cpp
	../client/renderer/models/mds.cpp
	../client/renderer/models/model.cpp
	../client/renderer/models/sp2.cpp
	../client/renderer/models/spr.cpp
	../client/renderer/backend.cpp
	../client/renderer/commands.cpp
	../client/renderer/curve.cpp
	../client/renderer/decals.cpp
	../client/renderer/driver_glx.cpp
	../client/renderer/flares.cpp
	../client/renderer/font.cpp
	../client/renderer/init.cpp
	../client/renderer/light.cpp
	../client/renderer/main.cpp
	../client/renderer/marks.cpp
	../client/renderer/noise.cpp
	../client/renderer/particle.cpp
	../client/renderer/qgl.cpp
	../client/renderer/scene.cpp
	../client/renderer/screenshot.cpp
	../client/renderer/shade.cpp
	../client/renderer/shade_calculate.cpp
	../client/renderer/shader.cpp
	../client/renderer/shadows.cpp
	../client/renderer/skin.cpp
	../client/renderer/sky.cpp
	../client/renderer/state.cpp
	../client/renderer/surface1.cpp
	../client/renderer/surface2.cpp
	../client/renderer/surface3.cpp
	../client/renderer/wad.cpp
	../client/renderer/world.cpp
	../client/sound/audiocd/linux.cpp
	../client/sound/midi/null.cpp
	../client/sound/driver_alsa.cpp
	../client/sound/main.cpp
	../client/sound/memory.cpp
	../client/sound/mix.cpp
	../client/ui/console.cpp
	../client/ui/draw.cpp
	../client/ui/screen.cpp
	../client/chase.cpp
	../client/client_main.cpp
	../client/system_unix.cpp
	../client/utils.cpp
)

set(WOLFCLIENT_ASM_FILES
	../client/sound/mix_x86.s
)

foreach(SRC ${WOLFCLIENT_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/../client/qasm.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(WOLFCLIENT_FILES ${WOLFCLIENT_FILES} ${OBJ})
endforeach(SRC)

add_library(wolfclient STATIC ${WOLFCLIENT_FILES})
set_target_properties(wolfclient PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(wolfclient core)
