add_library(progsvm STATIC
	main.cpp
)
set_target_properties(progsvm PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(progsvm core)
