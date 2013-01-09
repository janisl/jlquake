add_executable(common_tests
	common_tests.cpp
	Hexen2EffectsRandomTest.cpp
	ListTest.cpp
	MathTests.cpp
	StrTest.cpp
)

ADD_CUSTOM_COMMAND(TARGET run_tests POST_BUILD
	COMMAND "common_tests")
ADD_CUSTOM_TARGET(tests "common_tests" DEPENDS common_tests COMMENT "Running CPPUNIT tests...")

target_link_libraries(common_tests ${SERVERS_LIBS} cppunit)
