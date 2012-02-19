set(WOLFCORE_FILES
	common_defs.cpp
	exception.cpp
	strings.cpp
)

add_library(wolfcore STATIC ${WOLFCORE_FILES})
set_target_properties(wolfcore PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
