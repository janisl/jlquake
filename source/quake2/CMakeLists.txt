set(ARCH i386)

set(BASE_CFLAGS "-Dstricmp=strcasecmp")

set(RELEASE_CFLAGS "${BASE_CFLAGS} -m486 -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2
	-malign-jumps=2 -malign-functions=2")

set(DEBUG_CFLAGS "${BASE_CFLAGS} -g")
set(LDFLAGS -ldl -lm)
set(GLLDFLAGS -L/usr/X11R6/lib -lGL -lX11 -lXext -lXxf86dga -lXxf86vm)

#############################################################################
# CLIENT/SERVER
#############################################################################

set(QUAKE2_OBJS
	client/cl_cin.c
	client/cl_ents.c
	client/cl_fx.c
	client/cl_input.c
	client/cl_inv.c
	client/cl_main.c
	client/cl_newfx.c
	client/cl_parse.c
	client/cl_pred.c
	client/cl_tent.c
	client/cl_scrn.c
	client/cl_view.c
	client/console.c
	client/keys.c
	client/menu.c
	client/snd_dma.c
	client/snd_mem.c
	client/snd_mix.c
	client/qmenu.c
	game/m_flash.c
	qcommon/cmd.c
	qcommon/cmodel.c
	qcommon/common.c
	qcommon/crc.c
	qcommon/cvar.c
	qcommon/files.c
	qcommon/md4.c
	qcommon/net_chan.c
	game/q_shared.c
	qcommon/pmove.c
	server/sv_ccmds.c
	server/sv_ents.c
	server/sv_game.c
	server/sv_init.c
	server/sv_main.c
	server/sv_send.c
	server/sv_user.c
	server/sv_world.c
	linux/cd_linux.c
	linux/q_shlinux.c
	linux/vid_menu.c
	linux/vid_so.c
	linux/snd_linux.c
	linux/sys_linux.c
	linux/glob.c
	linux/net_udp.c
	ref_gl/gl_draw.c
	ref_gl/gl_image.c
	ref_gl/gl_light.c
	ref_gl/gl_mesh.c
	ref_gl/gl_model.c
	ref_gl/gl_rmain.c
	ref_gl/gl_rmisc.c
	ref_gl/gl_rsurf.c
	ref_gl/gl_warp.c
	linux/qgl_linux.c
	linux/gl_glx.c
)

set(QUAKE2_ASM_FILES
	linux/snd_mixa.s
)

foreach(SRC ${QUAKE2_ASM_FILES})
	get_filename_component(FILE_BASE ${SRC} NAME_WE)
	set(OBJ ${CMAKE_CURRENT_BINARY_DIR}/${FILE_BASE}.o)
	add_custom_command(OUTPUT ${OBJ}
		DEPENDS ${SRC} ${CMAKE_CURRENT_SOURCE_DIR}/linux/qasm.h
		COMMAND ${CMAKE_C_COMPILER} -x assembler-with-cpp -DELF
			-c ${CMAKE_CURRENT_SOURCE_DIR}/${SRC} -o ${OBJ}
	)
	set(QUAKE2_OBJS ${QUAKE2_OBJS} ${OBJ})
endforeach(SRC)

add_executable(vquake2 ${QUAKE2_OBJS})
set_target_properties(vquake2 PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(vquake2 ${GLLDFLAGS})

#############################################################################
# GAME
#############################################################################

set(GAME_OBJS
	game/g_ai.c
	game/p_client.c
	game/g_chase.c
	game/g_cmds.c
	game/g_svcmds.c
	game/g_combat.c
	game/g_func.c
	game/g_items.c
	game/g_main.c
	game/g_misc.c
	game/g_monster.c
	game/g_phys.c
	game/g_save.c
	game/g_spawn.c
	game/g_target.c
	game/g_trigger.c
	game/g_turret.c
	game/g_utils.c
	game/g_weapon.c
	game/m_actor.c
	game/m_berserk.c
	game/m_boss2.c
	game/m_boss3.c
	game/m_boss31.c
	game/m_boss32.c
	game/m_brain.c
	game/m_chick.c
	game/m_flipper.c
	game/m_float.c
	game/m_flyer.c
	game/m_gladiator.c
	game/m_gunner.c
	game/m_hover.c
	game/m_infantry.c
	game/m_insane.c
	game/m_medic.c
	game/m_move.c
	game/m_mutant.c
	game/m_parasite.c
	game/m_soldier.c
	game/m_supertank.c
	game/m_tank.c
	game/p_hud.c
	game/p_trail.c
	game/p_view.c
	game/p_weapon.c
	game/q_shared.c
	game/m_flash.c
)

add_library(q2game SHARED ${GAME_OBJS})
set_target_properties(q2game PROPERTIES CLEAN_DIRECT_OUTPUT 1 OUTPUT_NAME "game${ARCH}")
set_target_properties(q2game PROPERTIES PREFIX "")
set_target_properties(q2game PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(q2game ${LDFLAGS})
