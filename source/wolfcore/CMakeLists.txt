set(WOLFCORE_FILES
	../core/Common.cpp
	common_defs.cpp
	../core/crc.cpp
	../core/exception.cpp
	../core/log.cpp
	../core/md4.cpp
	../core/memory.cpp
	../core/strings.cpp
)

add_library(wolfcore STATIC ${WOLFCORE_FILES})
set_target_properties(wolfcore PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
