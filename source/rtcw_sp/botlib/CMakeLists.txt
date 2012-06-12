add_library(botlib-wolfsp STATIC
	be_ai_move.cpp
	be_interface.cpp
)
set_target_properties(botlib-wolfsp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
