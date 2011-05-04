set(BASE_CFLAGS "-Wall -pipe -fsigned-char")
set(DEBUG_CFLAGS "${BASE_CFLAGS} -g")
set(RELEASE_CFLAGS "${BASE_CFLAGS} -DNDEBUG -march=pentiumpro -O6 -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce")
set(ALL_CFLAGS ${DEBUG_CFLAGS})

set(CLIENTS_LIBS core client zlib jpeg asound X11 Xext Xxf86vm Xxf86dga GL)
set(SERVERS_LIBS core zlib dl)

add_subdirectory(core)
add_subdirectory(client)
add_subdirectory(progsvm)

add_subdirectory(quake)
add_subdirectory(quakeworld)
add_subdirectory(hexen2)
add_subdirectory(hexenworld)
add_subdirectory(quake2)
add_subdirectory(quake3)
