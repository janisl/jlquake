add_library(botlib-wolfmp STATIC
	be_interface.cpp
)
set_target_properties(botlib-wolfmp PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
