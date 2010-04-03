project(vquake)

cmake_minimum_required(VERSION 2.4)
if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

option(ENABLE_ZONE_DEBUG "Enable memory allocation debuging")

#
#  **** Add debug / development flags ****
#

if(ENABLE_ZONE_DEBUG)
	add_definitions(-DZONE_DEBUG=1)
endif(ENABLE_ZONE_DEBUG)

#
#  **** Check for inttypes.h ****
#

find_path(INTTYPES_INCLUDE_DIR inttypes.h)
if(INTTYPES_INCLUDE_DIR)
	add_definitions(-DHAVE_INTTYPES_H=1)
endif(INTTYPES_INCLUDE_DIR)

add_subdirectory(libs)
add_subdirectory(source)
