# don't wrap gcc messages
#BASECPPFLAGS.append( '-fmessage-length=0' )

#	BASECPPFLAGS.append( '-D_DEBUG' )
		# -fomit-frame-pointer: gcc manual indicates -O sets this implicitely,
		# only if that doesn't affect debugging
		# on Linux, this affects backtrace capability, so I'm assuming this is needed
		# -finline-functions: implicit at -O3
		# -fschedule-insns2: implicit at -O3
		# -funroll-loops ?
		# -mfpmath=sse -msse ?
		#OPTCPPFLAGS = [ '-O3', '-march=i686', '-Winline', '-ffast-math', '-fomit-frame-pointer', '-finline-functions', '-fschedule-insns2' ]

# maintain this dangerous optimization off at all times
#g_env.Append( CPPFLAGS = '-fno-strict-aliasing' )

set (ET_FILES
	qcommon/common.cpp
	qcommon/files.cpp

	client/cl_main.cpp

	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp
)

add_executable(jlet ${ET_FILES})
set_target_properties(jlet PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O")
target_link_libraries(jlet ${CLIENTS_LIBS})

#local_env.Append( CPPDEFINES = [ 'MASTER_SERVER_NAME=\\"%s\\"' % MASTER ] )

set (ETDED_FILES
	qcommon/common.cpp
	qcommon/files.cpp

	unix/linux_signals.cpp
	unix/unix_main.cpp
	unix/unix_shared.cpp

	null/null_client.cpp
)

add_executable(jletded ${ETDED_FILES})
set_target_properties(jletded PROPERTIES COMPILE_FLAGS "${ALL_CFLAGS} -O -DDEDICATED")
target_link_libraries(jletded ${SERVERS_LIBS} m)
