add_library(botlib-et STATIC
	be_ai_chat.cpp
	be_ai_move.cpp
	be_interface.cpp
)
set_target_properties(botlib-et PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
