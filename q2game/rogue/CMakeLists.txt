if(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(ARCH i386)
else(${SIZEOF_VOID_PTR} MATCHES "^4$")
	set(ARCH x86_64)
endif(${SIZEOF_VOID_PTR} MATCHES "^4$")

set(BASE_CFLAGS "")

set(RELEASE_CFLAGS "${BASE_CFLAGS} -m486 -O6 -ffast-math -funroll-loops
	-fomit-frame-pointer -fexpensive-optimizations -malign-loops=2
	-malign-jumps=2 -malign-functions=2")

set(DEBUG_CFLAGS "${BASE_CFLAGS} -g")
set(LDFLAGS -ldl -lm)

set(ROGUE_OBJS
	dm_ball.c
	dm_tag.c
	g_ai.c
	g_chase.c
	g_cmds.c
	g_combat.c
	g_func.c
	g_items.c
	g_main.c
	g_misc.c
	g_monster.c
	g_newai.c
	g_newdm.c
	g_newfnc.c
	g_newtarg.c
	g_newtrig.c
	g_newweap.c
	g_phys.c
	g_save.c
	g_spawn.c
	g_sphere.c
	g_svcmds.c
	g_target.c
	g_trigger.c
	g_turret.c
	g_utils.c
	g_weapon.c
	m_actor.c
	m_berserk.c
	m_boss2.c
	m_boss31.c
	m_boss32.c
	m_boss3.c
	m_brain.c
	m_carrier.c
	m_chick.c
	m_flash.c
	m_flipper.c
	m_float.c
	m_flyer.c
	m_gladiator.c
	m_gunner.c
	m_hover.c
	m_infantry.c
	m_insane.c
	m_medic.c
	m_move.c
	m_mutant.c
	m_parasite.c
	m_soldier.c
	m_stalker.c
	m_supertank.c
	m_tank.c
	m_turret.c
	m_widow2.c
	m_widow.c
	p_client.c
	p_hud.c
	p_trail.c
	p_view.c
	p_weapon.c
	q_shared.c
)

add_library(q2rogue SHARED ${ROGUE_OBJS})
set_target_properties(q2rogue PROPERTIES CLEAN_DIRECT_OUTPUT 1 OUTPUT_NAME "game${ARCH}")
set_target_properties(q2rogue PROPERTIES PREFIX "")
set_target_properties(q2rogue PROPERTIES COMPILE_FLAGS ${DEBUG_CFLAGS})
target_link_libraries(q2rogue ${LDFLAGS})
