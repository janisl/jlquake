add_library(null_client STATIC
	main.cpp
)
set_target_properties(null_client PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(null_client common)
