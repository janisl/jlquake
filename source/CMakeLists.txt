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

add_subdirectory(common)
add_subdirectory(client)
add_subdirectory(null_client)
add_subdirectory(server)
add_subdirectory(apps)
