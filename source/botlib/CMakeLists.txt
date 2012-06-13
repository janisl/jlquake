add_library(botlib STATIC
	be_interface.cpp
)
set_target_properties(botlib PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(botlib common)
