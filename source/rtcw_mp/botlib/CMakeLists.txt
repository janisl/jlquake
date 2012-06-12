add_library(botlib-wolfmp STATIC
	be_ai_chat.cpp
	be_ai_move.cpp
	be_ea.cpp
	be_interface.cpp
)
set_target_properties(botlib-wolfmp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
