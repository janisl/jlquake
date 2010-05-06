set(ARCH i386)

set(BASE_CFLAGS "")

set(RELEASE_CFLAGS "${BASE_CFLAGS} -m486 -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2
	-malign-jumps=2 -malign-functions=2")

set(DEBUG_CFLAGS "${BASE_CFLAGS} -g")
set(LDFLAGS -ldl -lm)
set(GLLDFLAGS -L/usr/X11R6/lib -lGL -lX11 -lXext -lXxf86dga -lXxf86vm -lasound)

#############################################################################
# CLIENT/SERVER
#############################################################################

set(QUAKE2_OBJS
	client/cl_cin.cpp
	client/cl_ents.cpp
	client/cl_fx.cpp
	client/cl_input.cpp
	client/cl_inv.cpp
	client/cl_main.cpp
	client/cl_newfx.cpp
	client/cl_parse.cpp
	client/cl_pred.cpp
	client/cl_tent.cpp
	client/cl_scrn.cpp
	client/cl_view.cpp
	client/console.cpp
	client/keys.cpp
	client/menu.cpp
	client/snd_dma.cpp
	client/qmenu.cpp
	qcommon/m_flash.cpp
	qcommon/cmd.cpp
	qcommon/cmodel.cpp
	qcommon/common.cpp
	qcommon/cvar.cpp
	qcommon/files.cpp
	qcommon/net_chan.cpp
	qcommon/q_shared.cpp
	qcommon/pmove.cpp
	server/sv_ccmds.cpp
	server/sv_ents.cpp
	server/sv_game.cpp
	server/sv_init.cpp
	server/sv_main.cpp
	server/sv_send.cpp
	server/sv_user.cpp
	server/sv_world.cpp
	linux/cd_linux.cpp
	linux/q_shlinux.cpp
	linux/vid_menu.cpp
	linux/vid_so.cpp
	linux/sys_linux.cpp
	linux/glob.cpp
	linux/net_udp.cpp
	ref_gl/gl_draw.cpp
	ref_gl/gl_image.cpp
	ref_gl/gl_light.cpp
	ref_gl/gl_mesh.cpp
	ref_gl/gl_model.cpp
	ref_gl/gl_rmain.cpp
	ref_gl/gl_rmisc.cpp
	ref_gl/gl_rsurf.cpp
	ref_gl/gl_warp.cpp
	linux/qgl_linux.cpp
	linux/gl_glx.cpp
)

add_executable(vquake2 ${QUAKE2_OBJS})
set_target_properties(vquake2 PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(vquake2 client core zlib ${GLLDFLAGS})
add_dependencies(vquake2 client)

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
