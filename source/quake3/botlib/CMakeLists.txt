set(BASE_CFLAGS "-pipe -fsigned-char -I/usr/X11R6/include")
set(DEBUG_CFLAGS "${BASE_CFLAGS} -g  -Wall -DNO_MOUSEGRAB -O")
set(RELEASE_CFLAGS "${BASE_CFLAGS} -DNDEBUG -O6 -mcpu=pentiumpro -march=pentium -fomit-frame-pointer -pipe -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce")

set(BOTLIB_SOURCES
	be_aas_bspq3.cpp
	be_aas_cluster.cpp
	be_aas_debug.cpp
	be_aas_entity.cpp
	be_aas_file.cpp
	be_aas_main.cpp
	be_aas_move.cpp
	be_aas_optimize.cpp
	be_aas_reach.cpp
	be_aas_route.cpp
	be_aas_routealt.cpp
	be_aas_sample.cpp
	be_ai_char.cpp
	be_ai_chat.cpp
	be_ai_gen.cpp
	be_ai_goal.cpp
	be_ai_move.cpp
	be_ai_weap.cpp
	be_ai_weight.cpp
	be_ea.cpp
	be_interface.cpp
	l_crc.cpp
	l_libvar.cpp
	l_log.cpp
	l_memory.cpp
	l_precomp.cpp
	l_script.cpp
	l_struct.cpp
)

add_library(botlib STATIC ${BOTLIB_SOURCES})
set_target_properties(botlib PROPERTIES COMPILE_FLAGS "${DEBUG_CFLAGS} -DBOTLIB")
add_dependencies(botlib core)
