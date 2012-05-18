add_library(server STATIC
	botlib/interface.cpp
	progsvm/edict.cpp
	progsvm/execute.cpp
	progsvm/main.cpp
)
set_target_properties(server PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
add_dependencies(server common)
