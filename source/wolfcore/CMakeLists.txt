set(WOLFCORE_FILES
	command_buffer.cpp
	../core/Common.cpp
	../core/common_defs.cpp
	console_variable.cpp
	../core/crc.cpp
	../core/endian.cpp
	../core/exception.cpp
	../core/files.cpp
	../core/info_string.cpp
	../core/log.cpp
	../core/md4.cpp
	../core/memory.cpp
	../core/strings.cpp
	../core/system_unix.cpp
	../core/unzip.cpp
)

add_library(wolfcore STATIC ${WOLFCORE_FILES})
set_target_properties(wolfcore PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS}")
