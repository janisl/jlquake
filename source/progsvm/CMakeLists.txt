add_library(progsvm STATIC
	edict.cpp
	main.cpp
)
set_target_properties(progsvm PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(progsvm core)
