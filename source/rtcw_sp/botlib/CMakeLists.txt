add_library(botlib-wolfsp STATIC
	be_aas_bspq3.cpp
	be_aas_debug.cpp
	be_aas_entity.cpp
	be_aas_main.cpp
	be_aas_move.cpp
	be_aas_reach.cpp
	be_aas_route.cpp
	be_aas_routetable.cpp
	be_aas_sample.cpp
	be_ai_chat.cpp
	be_ai_goal.cpp
	be_ai_move.cpp
	be_ea.cpp
	be_interface.cpp
)
set_target_properties(botlib-wolfsp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
