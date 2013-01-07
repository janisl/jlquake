project(jlquake)

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/")

option(ENABLE_MEM_DEBUG "Enable memory allocation debuging")

#
#  **** Add debug / development flags ****
#

if(ENABLE_MEM_DEBUG)
	add_definitions(-DMEM_DEBUG=1)
endif(ENABLE_MEM_DEBUG)

#
#  **** Check for inttypes.h ****
#

find_path(INTTYPES_INCLUDE_DIR inttypes.h)
if(INTTYPES_INCLUDE_DIR)
	add_definitions(-DHAVE_INTTYPES_H=1)
endif(INTTYPES_INCLUDE_DIR)

#
#  **** Check for size of void* (to detect 64 bit systems) ****
#

include(CheckTypeSize)
check_type_size(void*  SIZEOF_VOID_PTR)

# NOTE TTimo using -fshort-enums increases warnings on enum issues
#   this is for debugging and QA ONLY, the flag has ABI issues (OpenGL headers)
#$COMMON_CFLAGS = $COMMON_CFLAGS . '-fshort-enums ';
# maintain this dangerous optimization off at all times
#g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

set(BASE_CFLAGS "-Wall -pipe -fsigned-char -fvisibility=hidden")
set(DEBUG_CFLAGS "${BASE_CFLAGS} -g")
set(RELEASE_CFLAGS "${BASE_CFLAGS} -DNDEBUG -march=pentiumpro -O6 -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce")
set(ALL_CFLAGS ${DEBUG_CFLAGS})

set(CLIENTS_LIBS server client common z jpeg asound curl X11 Xext Xxf86vm Xxf86dga GL freetype pthread dl)
set(SERVERS_LIBS server common null_client z pthread dl)

include_directories(/usr/include/freetype2)

add_subdirectory(source)
add_subdirectory(tests)
