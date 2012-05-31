add_library(server STATIC
	botlib/aas_bsp.cpp
	botlib/aas_cluster.cpp
	botlib/aas_debug.cpp
	botlib/aas_entity.cpp
	botlib/aas_file.cpp
	botlib/aas_main.cpp
	botlib/aas_move.cpp
	botlib/aas_reach.cpp
	botlib/aas_route.cpp
	botlib/aas_routealt.cpp
	botlib/aas_sample.cpp
	botlib/ai_character.cpp
	botlib/ai_chat.cpp
	botlib/ai_genetic.cpp
	botlib/ai_goal.cpp
	botlib/ai_move.cpp
	botlib/ai_weapon.cpp
	botlib/ai_weight.cpp
	botlib/ea.cpp
	botlib/interface.cpp
	botlib/libvar.cpp
	botlib/log.cpp
	botlib/struct.cpp
	progsvm/edict.cpp
	progsvm/execute.cpp
	progsvm/main.cpp
)
set_target_properties(server PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(server common)
