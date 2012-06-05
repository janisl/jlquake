add_library(botlib STATIC
	be_aas_bspq3.cpp
	be_aas_debug.cpp
	be_aas_main.cpp
	be_aas_reach.cpp
	be_aas_route.cpp
	be_ai_chat.cpp
	be_ai_goal.cpp
	be_ai_move.cpp
	be_ea.cpp
	be_interface.cpp
)
set_target_properties(botlib PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(botlib common)
