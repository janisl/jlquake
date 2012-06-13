add_library(botlib-et STATIC
	be_interface.cpp
)
set_target_properties(botlib-et PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
