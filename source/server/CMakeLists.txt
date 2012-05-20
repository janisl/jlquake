add_library(server STATIC
	botlib/ai_character.cpp
	botlib/ai_genetic.cpp
	botlib/ai_goal.cpp
	botlib/ai_weight.cpp
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
