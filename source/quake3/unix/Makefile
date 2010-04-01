#
# Quake3 Unix Makefile
#
# Currently build for the following:
#	Linux i386 (full client)
#	Linux Alpha (dedicated server only) (TTimo: dropped)
#	FreeBSD i386 (dedicated server only) (TTimo: maybe worth updating)
#	SGI IRIX (full client) (TTimo: who's in charge - dropped otherwise)
#   Linux PPC (full client) (TTimo: dropped)
#
# Nov '98 by Zoid <zoid@idsoftware.com>
#
# Loki Hacking by Bernd Kreimeier 
#  and a little more by Ryan C. Gordon.
#  and a little more by Rafael Barrero
#
# GNU Make required
#

PLATFORM=$(shell uname|tr A-Z a-z)
PLATFORM_RELEASE=$(shell uname -r)

###
### These paths are where you probably want to change things
###

# Where we are building from (where the source code should be!)
MOUNT_DIR=../

# Where we are building to, libMesaVoodooGL.so.3.3 should be here, etc.
# the demo pk3 file should be here in demoq3/pak0.pk3 or baseq3/pak0.pk3
BDIR=$(MOUNT_DIR)/../run


# Build name
# BUILD_NAME=$(BUILD_NAME)
BUILD_NAME=quake3.x86



#############################################################################
##
## You shouldn't have to touch anything below here
##
#############################################################################

BASEQ3_DIR=$(BDIR)/baseq3

BD=debug$(ARCH)$(GLIBC)
BR=release$(ARCH)$(GLIBC)
CDIR=$(MOUNT_DIR)/client
SDIR=$(MOUNT_DIR)/server
RDIR=$(MOUNT_DIR)/renderer
CMDIR=$(MOUNT_DIR)/qcommon
UDIR=$(MOUNT_DIR)/unix
GDIR=$(MOUNT_DIR)/game
CGDIR=$(MOUNT_DIR)/cgame
BAIDIR=$(GDIR)
BLIBDIR=$(MOUNT_DIR)/botlib
NDIR=$(MOUNT_DIR)/null
UIDIR=$(MOUNT_DIR)/ui
Q3UIDIR=$(MOUNT_DIR)/q3_ui
FTDIR=$(MOUNT_DIR)/ft2
JPDIR=$(MOUNT_DIR)/jpeg-6
SPLNDIR=$(MOUNT_DIR)/splines

# extract version info
VERSION=$(shell ./extract_ver.pl)
RPM_RELEASE=1

# NOTE: used by dcp rcp targets, not referenced
#TESTDIR=/home/timo/Id/Quake3-loki/run

#############################################################################
# SETUP AND BUILD -- LINUX
#############################################################################

## Defaults
DLL_ONLY=false
# bk010215 - TODO - add all defaults / kill Ryan


ifeq ($(PLATFORM),linux)

  ifneq (,$(findstring libc6,$(shell if [ -e /lib/libc.so.6* ];then echo libc6;fi)))
    GLIBC=-glibc
  else
    GLIBC=
  endif #libc6 test

  ifneq (,$(findstring alpha,$(shell uname -m)))
    ARCH=axp
    RPMARCH=alpha
    VENDOR=dec
  else
    ifneq (,$(findstring ppc,$(shell uname -m)))
      #used for linux builds
      MESADIR=/usr/src/DRI-CVS/xc/xc/
      ARCH=ppc
      RPMARCH=ppc
      VENDOR=unknown
      DLL_ONLY=true
    else #default to i386
      MESADIR=../Mesa/
      ARCH=i386
      RPMARCH=i386
      VENDOR=unknown
      DLL_ONLY=false
    endif
  endif

  # bk001205: no mo'  -I/usr/include/glide, no FX
  # bk001205: no mo'  -Dstricmp=strcasecmp, see q_shared.h
  BASE_CFLAGS =  -pipe -fsigned-char
  # rcg010216: DLL_ONLY for PPC
  ifeq ($(strip $(DLL_ONLY)),true)
    BASE_CFLAGS += -DDLL_ONLY
  endif


  #GL_CFLAGS = -I$(MESADIR)/include -I/usr/X11R6/include
  GL_CFAGS = -I/usr/X11R6/include

  # bk001204 - need -O for -Wall for uninitialized
  # bk001205 - took out -O to get assertions (NDEBUG)
  # bk001206 - MALLOC_CHECK in addition to ZONE_DEBUG
  # TTimo 03/30/2001 temporary took out -Werror for initial merge
  #DEBUG_CFLAGS = $(BASE_CFLAGS) -g  -Wall -Werror
  DEBUG_CFLAGS = $(BASE_CFLAGS) -g  -Wall
  DEBUG_CFLAGS += -DNO_MOUSEGRAB
  DEBUG_CFLAGS += -O
#  DEBUG_CFLAGS=$(BASE_CFLAGS) -g  -Wall -O
  ifeq ($(ARCH),axp)
    CC=pgcc
    RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations
  else
    ifeq ($(ARCH),ppc)
      NEWPGCC=/loki/global/ppc/bin/gcc
      CC=$(NEWPGCC)
      RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -fomit-frame-pointer -pipe -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce
    else
      #NEWPGCC=/usr/local/gcc-2.95.2/bin/gcc # bk001205
      #NEWPGCC=/loki/global/x86/bin/gcc
      NEWPGCC=/usr/bin/gcc
      CC=$(shell if [ -f $(NEWPGCC) ]; then echo $(NEWPGCC); else echo pgcc; fi )
      CXX=/usr/bin/g++
# TTimo: legacy RELEASE_CFLAGS
# NOTE: the -fomit-frame-pointer option leads to an unstable binary on my test box if it was built on the main box
#   but building on the Mdk 7.2 baseline seems to work
      RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -mcpu=pentiumpro -march=pentium -fomit-frame-pointer -pipe -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce
# TTimo: use this for building on P3 gcc 2.95.3 libc2.2 for all targets (experimental! -fomit-fram-pointer removed)
#      RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -mcpu=pentiumpro -march=pentium -pipe -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing -fstrength-reduce
    endif
  endif

  LIBEXT=a

  SHLIBEXT=so
  SHLIBCFLAGS=-fPIC
  SHLIBLDFLAGS=-shared $(LDFLAGS)

  ARFLAGS=ar rv
  RANLIB=ranlib

  THREAD_LDFLAGS=-lpthread
  LDFLAGS=-ldl -lm
  GLLDFLAGS=-L/usr/X11R6/lib -lX11 -lXext -lXxf86dga -lXxf86vm

  ifeq ($(ARCH),axp)
    TARGETS=\
	  $(B)/$(PLATFORM)q3ded
  else
    TARGETS=\
	  $(B)/$(PLATFORM)quake3 \
	  $(B)/$(PLATFORM)quake3-smp \
	  $(B)/$(PLATFORM)q3ded \
	  $(B)/baseq3/cgame$(ARCH).$(SHLIBEXT) \
	  $(B)/baseq3/qagame$(ARCH).$(SHLIBEXT) \
	  $(B)/missionpack/cgame$(ARCH).$(SHLIBEXT) \
	  $(B)/missionpack/qagame$(ARCH).$(SHLIBEXT)
  endif

else # ifeq Linux

#############################################################################
# SETUP AND BUILD -- FREEBSD
#############################################################################

ifeq ($(PLATFORM),freebsd)

GLIBC= #libc is irrelevant

ifneq (,$(findstring alpha,$(shell uname -m)))
ARCH=axp
RPMARCH=alpha
VENDOR=dec
else #default to i386
ARCH=i386
RPMARCH=i386
VENDOR=unknown
endif #alpha test


BASE_CFLAGS =  -pipe

GL_CFLAGS = -I$(MESADIR)/include -I/usr/X11R6/include

DEBUG_CFLAGS=$(BASE_CFLAGS) -g  -Wall -Werror
ifeq ($(ARCH),axp)
CC=pgcc
RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -ffast-math -funroll-loops -fomit-frame-pointer -fexpensive-optimizations
else
#NEWPGCC=/loki/global/x86/bin/gcc # raistlin012301
#NEWPGCC=/usr/local/gcc-2.95.2/bin/gcc
NEWPGCC=/home/raistllin/src/gcc/build/install/bin/gcc
CC=$(shell if [ -f $(NEWPGCC) ]; then echo $(NEWPGCC); else echo pgcc; fi )
RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O6 -mcpu=pentiumpro -march=pentium -fomit-frame-pointer -pipe -ffast-math -malign-loops=2 -malign-jumps=2 -malign-functions=2 -fno-strict-aliasing - fstrength-reduce
endif

LIBEXT=a

SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared $(LDFLAGS)

ARFLAGS=ar rv
RANLIB=ranlib

THREAD_LDFLAGS=-lpthread
# don't need -ldl (FreeBSD)
LDFLAGS=-lm
#GLLDFLAGS=-L/usr/X11R6/lib -L$(MESADIR)/lib -lGL -lX11 -lXext -lXxf86dga -lXxf86vm
#GLLDFLAGS=-L/usr/X11/lib -lGL -lX11 -lXext -lm
GLLDFLAGS=-L/usr/X11R6/lib -lGL -lX11 -lXext -lXxf86dga -lXxf86vm

ifeq ($(ARCH),axp)
TARGETS=\
        $(B)/$(PLATFORM)q3ded
else
TARGETS=\
	$(B)/$(PLATFORM)quake3 \
	$(B)/baseq3/cgame$(ARCH).$(SHLIBEXT) \
	$(B)/baseq3/qagame$(ARCH).$(SHLIBEXT) \
	$(B)/missionpack/cgame$(ARCH).$(SHLIBEXT) \
	$(B)/missionpack/qagame$(ARCH).$(SHLIBEXT)
endif

else # ifeq freebsd

#############################################################################
# SETUP AND BUILD -- GENERIC
#############################################################################

#CC=egcs
#BASE_CFLAGS=-Dstricmp=strcasecmp
#DEBUG_CFLAGS=$(BASE_CFLAGS) -g 
#RELEASE_CFLAGS=$(BASE_CFLAGS) -DNDEBUG -O3

LIBEXT=a

SHLIBEXT=so
SHLIBCFLAGS=-fPIC
SHLIBLDFLAGS=-shared

ARFLAGS=ar rv
RANLIB=ranlib

ifeq ($(PLATFORM),freebsd)
LDFLAGS=-lm
else
LDFLAGS=-ldl -lm
endif # ifeq freebsd

TARGETS=\
	$(B)/$(PLATFORM)q3ded

endif #Linux
endif #FreeBSD

DO_CC=$(CC) $(CFLAGS) -o $@ -c $<
DO_CXX=$(CXX) $(CFLAGS) -o $@ -c $<
DO_SMP_CC=$(CC) $(CFLAGS) -DSMP -o $@ -c $<
DO_BOT_CC=$(CC) $(CFLAGS) -DBOTLIB  -o $@ -c $<   # $(SHLIBCFLAGS) # bk001212
DO_DEBUG_CC=$(CC) $(DEBUG_CFLAGS) -o $@ -c $<
DO_SHLIB_CC=$(CC) $(CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<
DO_SHLIB_DEBUG_CC=$(CC) $(DEBUG_CFLAGS) $(SHLIBCFLAGS) -o $@ -c $<
DO_AS=$(CC) $(CFLAGS) -DELF -x assembler-with-cpp -o $@ -c $<
DO_NASM=nasm -f elf -o $@  $<
DO_DED_CC=$(CC) -DDEDICATED -DC_ONLY $(CFLAGS) -o $@ -c $<

#DO_LCC=$(LCC) -o $@ -S -Wf-target=bytecode -Wf-g -DQ3_VM -I$(CGDIR) -I$(GDIR)  -I$(UIDIR)  $< 

#### DEFAULT TARGET
default:build_debug

debug: build_debug
release: build_release

build_debug:
	$(MAKE)  targets B=$(BD) CFLAGS="$(DEBUG_CFLAGS)"

build_release:
	$(MAKE)  targets B=$(BR) CFLAGS="$(RELEASE_CFLAGS)"

#Build both debug and release builds
all:build_debug build_release

targets:makedirs $(TARGETS)

makedirs:
	@if [ ! -d $(B) ];then mkdir $(B);fi
	@if [ ! -d $(B)/client ];then mkdir $(B)/client;fi
	@if [ ! -d $(B)/ded ];then mkdir $(B)/ded;fi
	@if [ ! -d $(B)/ref ];then mkdir $(B)/ref;fi
	@if [ ! -d $(B)/ft2 ];then mkdir $(B)/ft2;fi
	@if [ ! -d $(B)/baseq3 ];then mkdir $(B)/baseq3;fi
	@if [ ! -d $(B)/baseq3/cgame ];then mkdir $(B)/baseq3/cgame;fi
	@if [ ! -d $(B)/baseq3/game ];then mkdir $(B)/baseq3/game;fi
	@if [ ! -d $(B)/baseq3/ui ];then mkdir $(B)/baseq3/ui;fi
	@if [ ! -d $(B)/missionpack ];then mkdir $(B)/missionpack;fi
	@if [ ! -d $(B)/missionpack/cgame ];then mkdir $(B)/missionpack/cgame;fi
	@if [ ! -d $(B)/missionpack/game ];then mkdir $(B)/missionpack/game;fi
	@if [ ! -d $(B)/q3static ];then mkdir $(B)/q3static;fi

#############################################################################
# CLIENT/SERVER
#############################################################################

Q3OBJ = \
	$(B)/client/cl_cgame.o \
	$(B)/client/cl_cin.o \
	$(B)/client/cl_console.o \
	$(B)/client/cl_input.o \
	$(B)/client/cl_keys.o \
	$(B)/client/cl_main.o \
	$(B)/client/cl_net_chan.o \
	$(B)/client/cl_parse.o \
	$(B)/client/cl_scrn.o \
	$(B)/client/cl_ui.o \
	\
	$(B)/client/cm_load.o \
	$(B)/client/cm_patch.o \
	$(B)/client/cm_polylib.o \
	$(B)/client/cm_test.o \
	$(B)/client/cm_trace.o \
	\
	$(B)/client/cmd.o \
	$(B)/client/common.o \
	$(B)/client/cvar.o \
	$(B)/client/files.o \
	$(B)/client/md4.o \
	$(B)/client/msg.o \
	$(B)/client/net_chan.o \
	$(B)/client/huffman.o \
	\
	$(B)/client/snd_adpcm.o \
	$(B)/client/snd_dma.o \
	$(B)/client/snd_mem.o \
	$(B)/client/snd_mix.o \
	$(B)/client/snd_wavelet.o \
	\
	$(B)/client/sv_bot.o \
	$(B)/client/sv_ccmds.o \
	$(B)/client/sv_client.o \
	$(B)/client/sv_game.o \
	$(B)/client/sv_init.o \
	$(B)/client/sv_main.o \
	$(B)/client/sv_net_chan.o \
	$(B)/client/sv_snapshot.o \
	$(B)/client/sv_world.o \
	\
	$(B)/client/q_math.o \
	$(B)/client/q_shared.o \
	\
	$(B)/client/unzip.o \
	$(B)/client/vm.o \
	$(B)/client/vm_interpreted.o \
	\
	$(B)/client/be_aas_bspq3.o \
	$(B)/client/be_aas_cluster.o \
	$(B)/client/be_aas_debug.o \
	$(B)/client/be_aas_entity.o \
	$(B)/client/be_aas_file.o \
	$(B)/client/be_aas_main.o \
	$(B)/client/be_aas_move.o \
	$(B)/client/be_aas_optimize.o \
	$(B)/client/be_aas_reach.o \
	$(B)/client/be_aas_route.o \
	$(B)/client/be_aas_routealt.o \
	$(B)/client/be_aas_sample.o \
	$(B)/client/be_ai_char.o \
	$(B)/client/be_ai_chat.o \
	$(B)/client/be_ai_gen.o \
	$(B)/client/be_ai_goal.o \
	$(B)/client/be_ai_move.o \
	$(B)/client/be_ai_weap.o \
	$(B)/client/be_ai_weight.o \
	$(B)/client/be_ea.o \
	$(B)/client/be_interface.o \
	$(B)/client/l_crc.o \
	$(B)/client/l_libvar.o \
	$(B)/client/l_log.o \
	$(B)/client/l_memory.o \
	$(B)/client/l_precomp.o \
	$(B)/client/l_script.o \
	$(B)/client/l_struct.o \
	\
	$(B)/client/jcapimin.o \
	$(B)/client/jchuff.o   \
	$(B)/client/jcinit.o \
	$(B)/client/jccoefct.o  \
	$(B)/client/jccolor.o \
	$(B)/client/jfdctflt.o \
	$(B)/client/jcdctmgr.o \
	$(B)/client/jcphuff.o \
	$(B)/client/jcmainct.o \
	$(B)/client/jcmarker.o \
	$(B)/client/jcmaster.o \
	$(B)/client/jcomapi.o \
	$(B)/client/jcparam.o \
	$(B)/client/jcprepct.o \
	$(B)/client/jcsample.o \
	$(B)/client/jdapimin.o \
	$(B)/client/jdapistd.o \
	$(B)/client/jdatasrc.o \
	$(B)/client/jdcoefct.o \
	$(B)/client/jdcolor.o \
	$(B)/client/jddctmgr.o \
	$(B)/client/jdhuff.o \
	$(B)/client/jdinput.o \
	$(B)/client/jdmainct.o \
	$(B)/client/jdmarker.o \
	$(B)/client/jdmaster.o \
	$(B)/client/jdpostct.o \
	$(B)/client/jdsample.o \
	$(B)/client/jdtrans.o \
	$(B)/client/jerror.o \
	$(B)/client/jidctflt.o \
	$(B)/client/jmemmgr.o \
	$(B)/client/jmemnobs.o \
	$(B)/client/jutils.o \
	\
	$(B)/client/tr_animation.o \
	$(B)/client/tr_backend.o \
	$(B)/client/tr_bsp.o \
	$(B)/client/tr_cmds.o \
	$(B)/client/tr_curve.o \
	$(B)/client/tr_flares.o \
	$(B)/client/tr_font.o \
	$(B)/client/tr_image.o \
	$(B)/client/tr_init.o \
	$(B)/client/tr_light.o \
	$(B)/client/tr_main.o \
	$(B)/client/tr_marks.o \
	$(B)/client/tr_mesh.o \
	$(B)/client/tr_model.o \
	$(B)/client/tr_noise.o \
	$(B)/client/tr_scene.o \
	$(B)/client/tr_shade.o \
	$(B)/client/tr_shade_calc.o \
	$(B)/client/tr_shader.o \
	$(B)/client/tr_shadows.o \
	$(B)/client/tr_sky.o \
	$(B)/client/tr_surface.o \
	$(B)/client/tr_world.o \
	\
	$(B)/client/unix_main.o \
	$(B)/client/unix_net.o \
	$(B)/client/unix_shared.o
#	\
#	$(B)/client/ahoptim.o  \
#	$(B)/client/autohint.o \
#	$(B)/client/ftbase.o   \
#	$(B)/client/ftdebug.o  \
#	$(B)/client/ftglyph.o  \
#	$(B)/client/ftmm.o     \
#	$(B)/client/ftsystem.o \
#	$(B)/client/raster1.o  \
#	$(B)/client/sfnt.o     \
#	$(B)/client/sfobjs.o   \
#	$(B)/client/smooth.o   \
#	$(B)/client/truetype.o
#	\
#	$(B)/client/q_parse.o \
#	$(B)/client/math_quaternion.o \
#	$(B)/client/util_str.o \
#	$(B)/client/math_angles.o \
#	$(B)/client/math_vector.o \
#	$(B)/client/splines.o \
#	$(B)/client/math_matrix.o

#	$(B)/client/q_shared.o \

    ifeq ($(ARCH),i386)
	  Q3OBJ += $(B)/client/vm_x86.o
    endif

    ifeq ($(ARCH),ppc)
      ifeq ($(DLL_ONLY),false)
        Q3OBJ += $(B)/client/vm_ppc.o
      endif
    endif

#	$(B)/client/jctrans.o \

#platform specific objects
ifeq ($(PLATFORM),freebsd)
	Q3POBJ=\
		$(B)/client/linux_common.o \
		$(B)/client/linux_qgl.o \
		$(B)/client/linux_glimp.o \
		$(B)/client/linux_signals.o \
		$(B)/client/linux_snd.o \
		$(B)/client/snd_mixa.o \
		$(B)/client/matha.o   \
		$(B)/client/ftol.o \
		$(B)/client/snapvector.o
else
ifeq ($(PLATFORM),irix)
	Q3POBJ=\
		$(B)/client/irix_qgl.o \
		$(B)/client/irix_glimp.o \
		$(B)/client/irix_snd.o
else
ifeq ($(PLATFORM),linux)
ifeq ($(ARCH),axp)
	Q3POBJ=
else
	Q3POBJ=\
		$(B)/client/linux_common.o \
		$(B)/client/linux_qgl.o \
		$(B)/client/linux_glimp.o \
		$(B)/client/linux_joystick.o \
		$(B)/client/linux_signals.o \
		$(B)/client/linux_snd.o \
		$(B)/client/snd_mixa.o \
		$(B)/client/matha.o \

	Q3POBJ_SMP=\
		$(B)/client/linux_common.o \
		$(B)/client/linux_qgl.o \
		$(B)/client/linux_glimp_smp.o \
		$(B)/client/linux_joystick.o \
		$(B)/client/linux_signals.o \
		$(B)/client/linux_snd.o \
		$(B)/client/snd_mixa.o \
		$(B)/client/matha.o

    ifeq ($(ARCH),i386)
		Q3POBJ += $(B)/client/ftol.o $(B)/client/snapvector.o
		Q3POBJ_SMP += $(B)/client/ftol.o $(B)/client/snapvector.o
    endif

endif
endif #Linux
endif #FreeBSD
endif #IRIX

$(B)/$(PLATFORM)quake3 : $(Q3OBJ) $(Q3POBJ)
	$(CC)  -o $@ $(Q3OBJ) $(Q3POBJ) $(GLLDFLAGS) $(LDFLAGS) 
# TTimo: splines code requires C++ linking, but splines have not been officially included in the codebase
#	$(CXX)  -o $@ $(Q3OBJ) $(Q3POBJ) $(GLLDFLAGS) $(LDFLAGS) 

$(B)/$(PLATFORM)quake3-smp : $(Q3OBJ) $(Q3POBJ_SMP)
	$(CC)  -o $@ $(Q3OBJ) $(Q3POBJ_SMP) $(GLLDFLAGS) \
		$(THREAD_LDFLAGS) $(LDFLAGS) 

$(B)/client/cl_cgame.o : $(CDIR)/cl_cgame.c; $(DO_CC)   
$(B)/client/cl_cin.o : $(CDIR)/cl_cin.c; $(DO_CC)       
$(B)/client/cl_console.o : $(CDIR)/cl_console.c; $(DO_CC)  
$(B)/client/cl_input.o : $(CDIR)/cl_input.c; $(DO_CC)   
$(B)/client/cl_keys.o : $(CDIR)/cl_keys.c; $(DO_CC)     
$(B)/client/cl_main.o : $(CDIR)/cl_main.c; $(DO_CC)     
$(B)/client/cl_net_chan.o : $(CDIR)/cl_net_chan.c; $(DO_CC)  
$(B)/client/cl_parse.o : $(CDIR)/cl_parse.c; $(DO_CC)   
$(B)/client/cl_scrn.o : $(CDIR)/cl_scrn.c; $(DO_CC)    
$(B)/client/cl_ui.o : $(CDIR)/cl_ui.c; $(DO_CC)         
$(B)/client/snd_adpcm.o : $(CDIR)/snd_adpcm.c; $(DO_CC)   
$(B)/client/snd_dma.o : $(CDIR)/snd_dma.c; $(DO_CC)       
$(B)/client/snd_mem.o : $(CDIR)/snd_mem.c; $(DO_CC)        
$(B)/client/snd_mix.o : $(CDIR)/snd_mix.c; $(DO_CC)       
$(B)/client/snd_wavelet.o : $(CDIR)/snd_wavelet.c; $(DO_CC)     
$(B)/client/sv_bot.o : $(SDIR)/sv_bot.c; $(DO_CC)        
$(B)/client/sv_client.o : $(SDIR)/sv_client.c; $(DO_CC)     
$(B)/client/sv_ccmds.o : $(SDIR)/sv_ccmds.c; $(DO_CC)       
$(B)/client/sv_game.o : $(SDIR)/sv_game.c; $(DO_CC)        
$(B)/client/sv_init.o : $(SDIR)/sv_init.c; $(DO_CC)        
$(B)/client/sv_main.o : $(SDIR)/sv_main.c; $(DO_CC)        
$(B)/client/sv_net_chan.o : $(SDIR)/sv_net_chan.c; $(DO_CC)     
$(B)/client/sv_snapshot.o : $(SDIR)/sv_snapshot.c; $(DO_CC)      
$(B)/client/sv_world.o : $(SDIR)/sv_world.c; $(DO_CC)         
$(B)/client/cm_trace.o : $(CMDIR)/cm_trace.c; $(DO_CC)       
$(B)/client/cm_load.o : $(CMDIR)/cm_load.c; $(DO_CC)        
$(B)/client/cm_test.o : $(CMDIR)/cm_test.c; $(DO_CC)        
$(B)/client/cm_patch.o : $(CMDIR)/cm_patch.c; $(DO_CC)       
$(B)/client/cm_polylib.o : $(CMDIR)/cm_polylib.c; $(DO_CC)     
$(B)/client/cmd.o : $(CMDIR)/cmd.c; $(DO_CC)                   
$(B)/client/common.o : $(CMDIR)/common.c; $(DO_CC)               
$(B)/client/cvar.o : $(CMDIR)/cvar.c; $(DO_CC)                
$(B)/client/files.o : $(CMDIR)/files.c; $(DO_CC)              
$(B)/client/md4.o : $(CMDIR)/md4.c; $(DO_CC)                   
$(B)/client/msg.o : $(CMDIR)/msg.c; $(DO_CC)                    
$(B)/client/net_chan.o : $(CMDIR)/net_chan.c; $(DO_CC)
$(B)/client/huffman.o : $(CMDIR)/huffman.c; $(DO_CC)
$(B)/client/q_shared.o : $(GDIR)/q_shared.c; $(DO_CC)         
$(B)/client/q_math.o : $(GDIR)/q_math.c; $(DO_CC)               

$(B)/client/be_aas_bspq3.o : $(BLIBDIR)/be_aas_bspq3.c; $(DO_BOT_CC) 
$(B)/client/be_aas_cluster.o : $(BLIBDIR)/be_aas_cluster.c; $(DO_BOT_CC) 
$(B)/client/be_aas_debug.o : $(BLIBDIR)/be_aas_debug.c; $(DO_BOT_CC) 
$(B)/client/be_aas_entity.o : $(BLIBDIR)/be_aas_entity.c; $(DO_BOT_CC) 
$(B)/client/be_aas_file.o : $(BLIBDIR)/be_aas_file.c; $(DO_BOT_CC) 
$(B)/client/be_aas_main.o : $(BLIBDIR)/be_aas_main.c; $(DO_BOT_CC) 
$(B)/client/be_aas_move.o : $(BLIBDIR)/be_aas_move.c; $(DO_BOT_CC) 
$(B)/client/be_aas_optimize.o : $(BLIBDIR)/be_aas_optimize.c; $(DO_BOT_CC) 
$(B)/client/be_aas_reach.o : $(BLIBDIR)/be_aas_reach.c; $(DO_BOT_CC) 
$(B)/client/be_aas_route.o : $(BLIBDIR)/be_aas_route.c; $(DO_BOT_CC) 
$(B)/client/be_aas_routealt.o : $(BLIBDIR)/be_aas_routealt.c; $(DO_BOT_CC) 
$(B)/client/be_aas_sample.o : $(BLIBDIR)/be_aas_sample.c; $(DO_BOT_CC) 
$(B)/client/be_ai_char.o : $(BLIBDIR)/be_ai_char.c; $(DO_BOT_CC) 
$(B)/client/be_ai_chat.o : $(BLIBDIR)/be_ai_chat.c; $(DO_BOT_CC) 
$(B)/client/be_ai_gen.o : $(BLIBDIR)/be_ai_gen.c; $(DO_BOT_CC) 
$(B)/client/be_ai_goal.o : $(BLIBDIR)/be_ai_goal.c; $(DO_BOT_CC) 
$(B)/client/be_ai_move.o : $(BLIBDIR)/be_ai_move.c; $(DO_BOT_CC) 
$(B)/client/be_ai_weap.o : $(BLIBDIR)/be_ai_weap.c; $(DO_BOT_CC) 
$(B)/client/be_ai_weight.o : $(BLIBDIR)/be_ai_weight.c; $(DO_BOT_CC) 
$(B)/client/be_ea.o : $(BLIBDIR)/be_ea.c; $(DO_BOT_CC) 
$(B)/client/be_interface.o : $(BLIBDIR)/be_interface.c; $(DO_BOT_CC) 
$(B)/client/l_crc.o : $(BLIBDIR)/l_crc.c; $(DO_BOT_CC) 
$(B)/client/l_libvar.o : $(BLIBDIR)/l_libvar.c; $(DO_BOT_CC) 
$(B)/client/l_log.o : $(BLIBDIR)/l_log.c; $(DO_BOT_CC) 
$(B)/client/l_memory.o : $(BLIBDIR)/l_memory.c; $(DO_BOT_CC) 
$(B)/client/l_precomp.o : $(BLIBDIR)/l_precomp.c; $(DO_BOT_CC) 
$(B)/client/l_script.o : $(BLIBDIR)/l_script.c; $(DO_BOT_CC) 
$(B)/client/l_struct.o : $(BLIBDIR)/l_struct.c; $(DO_BOT_CC) 

$(B)/client/jcapimin.o : $(JPDIR)/jcapimin.c; $(DO_CC) 
$(B)/client/jchuff.o : $(JPDIR)/jchuff.c; $(DO_CC) 
$(B)/client/jcinit.o : $(JPDIR)/jcinit.c; $(DO_CC) 
$(B)/client/jccoefct.o : $(JPDIR)/jccoefct.c; $(DO_CC) 
$(B)/client/jccolor.o : $(JPDIR)/jccolor.c; $(DO_CC) 
$(B)/client/jfdctflt.o : $(JPDIR)/jfdctflt.c; $(DO_CC) 
$(B)/client/jcdctmgr.o : $(JPDIR)/jcdctmgr.c; $(DO_CC) 
$(B)/client/jcmainct.o : $(JPDIR)/jcmainct.c; $(DO_CC) 
$(B)/client/jcmarker.o : $(JPDIR)/jcmarker.c; $(DO_CC) 
$(B)/client/jcmaster.o : $(JPDIR)/jcmaster.c; $(DO_CC) 
$(B)/client/jcomapi.o : $(JPDIR)/jcomapi.c; $(DO_CC)
$(B)/client/jcparam.o : $(JPDIR)/jcparam.c;  $(DO_CC)
$(B)/client/jcprepct.o : $(JPDIR)/jcprepct.c; $(DO_CC)
$(B)/client/jcsample.o : $(JPDIR)/jcsample.c; $(DO_CC) 

$(B)/client/jdapimin.o : $(JPDIR)/jdapimin.c; $(DO_CC) 
$(B)/client/jdapistd.o : $(JPDIR)/jdapistd.c; $(DO_CC) 
$(B)/client/jdatasrc.o : $(JPDIR)/jdatasrc.c; $(DO_CC) 
$(B)/client/jdcoefct.o : $(JPDIR)/jdcoefct.c; $(DO_CC) 
$(B)/client/jdcolor.o : $(JPDIR)/jdcolor.c; $(DO_CC) 
$(B)/client/jcphuff.o : $(JPDIR)/jcphuff.c; $(DO_CC) 
$(B)/client/jddctmgr.o : $(JPDIR)/jddctmgr.c; $(DO_CC) 
$(B)/client/jdhuff.o : $(JPDIR)/jdhuff.c; $(DO_CC) 
$(B)/client/jdinput.o : $(JPDIR)/jdinput.c; $(DO_CC) 
$(B)/client/jdmainct.o : $(JPDIR)/jdmainct.c; $(DO_CC) 
$(B)/client/jdmarker.o : $(JPDIR)/jdmarker.c; $(DO_CC) 
$(B)/client/jdmaster.o : $(JPDIR)/jdmaster.c; $(DO_CC) 
$(B)/client/jdpostct.o : $(JPDIR)/jdpostct.c; $(DO_CC) 
$(B)/client/jdsample.o : $(JPDIR)/jdsample.c; $(DO_CC) 
$(B)/client/jdtrans.o : $(JPDIR)/jdtrans.c; $(DO_CC) 
$(B)/client/jerror.o : $(JPDIR)/jerror.c; $(DO_CC) $(GL_CFLAGS) 
$(B)/client/jidctflt.o : $(JPDIR)/jidctflt.c; $(DO_CC) 
$(B)/client/jmemmgr.o : $(JPDIR)/jmemmgr.c; $(DO_CC) 
$(B)/client/jmemnobs.o : $(JPDIR)/jmemnobs.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/jutils.o : $(JPDIR)/jutils.c; $(DO_CC) 

$(B)/client/tr_bsp.o : $(RDIR)/tr_bsp.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_animation.o : $(RDIR)/tr_animation.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_backend.o : $(RDIR)/tr_backend.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_cmds.o : $(RDIR)/tr_cmds.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_curve.o : $(RDIR)/tr_curve.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_flares.o : $(RDIR)/tr_flares.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_font.o : $(RDIR)/tr_font.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_image.o : $(RDIR)/tr_image.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_init.o : $(RDIR)/tr_init.c; $(DO_CC)    $(GL_CFLAGS) 
$(B)/client/tr_light.o : $(RDIR)/tr_light.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_main.o : $(RDIR)/tr_main.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_marks.o : $(RDIR)/tr_marks.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_mesh.o : $(RDIR)/tr_mesh.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_model.o : $(RDIR)/tr_model.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_noise.o : $(RDIR)/tr_noise.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_scene.o : $(RDIR)/tr_scene.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_shade.o : $(RDIR)/tr_shade.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_shader.o : $(RDIR)/tr_shader.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_shade_calc.o : $(RDIR)/tr_shade_calc.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_shadows.o : $(RDIR)/tr_shadows.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_sky.o : $(RDIR)/tr_sky.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_smp.o : $(RDIR)/tr_smp.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_stripify.o : $(RDIR)/tr_stripify.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_subdivide.o : $(RDIR)/tr_subdivide.c; $(DO_CC)   $(GL_CFLAGS) 
$(B)/client/tr_surface.o : $(RDIR)/tr_surface.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/tr_world.o : $(RDIR)/tr_world.c; $(DO_CC)   $(GL_CFLAGS) 

$(B)/client/unix_qgl.o : $(UDIR)/unix_qgl.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/unix_main.o : $(UDIR)/unix_main.c; $(DO_CC) 
$(B)/client/unix_net.o : $(UDIR)/unix_net.c; $(DO_CC) 
$(B)/client/unix_shared.o : $(UDIR)/unix_shared.c; $(DO_CC) 
$(B)/client/irix_glimp.o : $(UDIR)/irix_glimp.c; $(DO_CC) 
$(B)/client/irix_glimp_smp.o : $(UDIR)/irix_glimp.c; $(DO_SMP_CC) 
$(B)/client/irix_snd.o : $(UDIR)/irix_snd.c; $(DO_CC) 
$(B)/client/irix_input.o : $(UDIR)/irix_input.c; $(DO_CC) 
$(B)/client/linux_common.o : $(UDIR)/linux_common.c; $(DO_CC)
$(B)/client/linux_glimp.o : $(UDIR)/linux_glimp.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/linux_glimp_smp.o : $(UDIR)/linux_glimp.c; $(DO_SMP_CC)  $(GL_CFLAGS) 
$(B)/client/linux_joystick.o : $(UDIR)/linux_joystick.c; $(DO_CC)  
$(B)/client/linux_qgl.o : $(UDIR)/linux_qgl.c; $(DO_CC)  $(GL_CFLAGS) 
$(B)/client/linux_input.o : $(UDIR)/linux_input.c; $(DO_CC) 
$(B)/client/linux_signals.o : $(UDIR)/linux_signals.c; $(DO_CC) 
$(B)/client/linux_snd.o : $(UDIR)/linux_snd.c; $(DO_CC) 
$(B)/client/snd_mixa.o : $(UDIR)/snd_mixa.s; $(DO_AS) 
$(B)/client/matha.o : $(UDIR)/matha.s; $(DO_AS) 

ifeq ($(ARCH),i386)
$(B)/client/ftol.o : $(UDIR)/ftol.nasm; $(DO_NASM)
$(B)/client/snapvector.o : $(UDIR)/snapvector.nasm; $(DO_NASM)
$(B)/client/vm_x86.o : $(CMDIR)/vm_x86.c; $(DO_CC) 
endif

ifeq ($(ARCH),ppc)
ifeq ($(DLL_ONLY),false)
$(B)/client/vm_ppc.o : $(CMDIR)/vm_ppc.c; $(DO_CC)
endif
endif

$(B)/client/unzip.o : $(CMDIR)/unzip.c; $(DO_CC) 
$(B)/client/vm.o : $(CMDIR)/vm.c; $(DO_CC) 
$(B)/client/vm_interpreted.o : $(CMDIR)/vm_interpreted.c; $(DO_CC) 

$(B)/client/ahoptim.o  : $(FTDIR)/ahoptim.c;   $(DO_CC)
$(B)/client/autohint.o : $(FTDIR)/autohint.c;  $(DO_CC)
$(B)/client/ftbase.o   : $(FTDIR)/ftbase.c;    $(DO_CC)
$(B)/client/ftdebug.o  : $(FTDIR)/ftdebug.c;   $(DO_CC)
$(B)/client/ftglyph.o  : $(FTDIR)/ftglyph.c;   $(DO_CC)
$(B)/client/ftinit.o   : $(FTDIR)/ftinit.c;    $(DO_CC)
$(B)/client/ftmm.o     : $(FTDIR)/ftmm.c;      $(DO_CC)
$(B)/client/ftsystem.o : $(FTDIR)/ftsystem.c;  $(DO_CC)
$(B)/client/raster1.o  : $(FTDIR)/raster1.c;   $(DO_CC) -DFT_FLAT_COMPILE
$(B)/client/sfnt.o     : $(FTDIR)/sfnt.c;      $(DO_CC)
$(B)/client/sfobjs.o   : $(FTDIR)/sfobjs.c;    $(DO_CC)
$(B)/client/smooth.o   : $(FTDIR)/smooth.c;    $(DO_CC) -DFT_FLAT_COMPILE
$(B)/client/truetype.o : $(FTDIR)/truetype.c;  $(DO_CC)

# TTimo: took out splines code
#$(B)/client/q_parse.o : $(SPLNDIR)/q_parse.cpp; $(DO_CXX)
#$(B)/client/math_quaternion.o : $(SPLNDIR)/math_quaternion.cpp; $(DO_CXX)
# TTimo: ignored
#$(B)/client/q_shared.o : $(SPLNDIR)/q_shared.cpp; $(DO_CXX)
#$(B)/client/util_str.o : $(SPLNDIR)/util_str.cpp; $(DO_CXX)
#$(B)/client/math_angles.o : $(SPLNDIR)/math_angles.cpp; $(DO_CXX)
#$(B)/client/math_vector.o : $(SPLNDIR)/math_vector.cpp; $(DO_CXX)
#$(B)/client/splines.o : $(SPLNDIR)/splines.cpp; $(DO_CXX)
#$(B)/client/math_matrix.o : $(SPLNDIR)/math_matrix.cpp; $(DO_CXX)

## Included by raster1.c
# $(B)/client/ftraster.o : $(FTDIR)/ftraster.c;  $(DO_CC)
# $(B)/client/ftrend1.o  : $(FTDIR)/ftrend1.c;   $(DO_CC)

## Included by autohint.c
# $(B)/client/ahangles.o : $(FTDIR)/ahangles.c;  $(DO_CC)
# $(B)/client/ahglyph.o  : $(FTDIR)/ahglyph.c;   $(DO_CC)
# $(B)/client/ahglobal.o : $(FTDIR)/ahglobal.c;  $(DO_CC)
# $(B)/client/ahhint.o   : $(FTDIR)/ahhint.c;    $(DO_CC)
# $(B)/client/ahmodule.o : $(FTDIR)/ahmodule.c;  $(DO_CC)

## Included by ftcalc.c
# $(B)/client/ftcalc.o   : $(FTDIR)/ftcalc.c;    $(DO_CC)
# $(B)/client/ftobjs.o   : $(FTDIR)/ftobjs.c;    $(DO_CC)
# $(B)/client/ftstream.o : $(FTDIR)/ftstream.c;  $(DO_CC)
# $(B)/client/ftlist.o   : $(FTDIR)/ftlist.c;    $(DO_CC)
# $(B)/client/ftoutln.o  : $(FTDIR)/ftoutln.c;   $(DO_CC)
# $(B)/client/ftextend.o : $(FTDIR)/ftextend.c;  $(DO_CC)
# $(B)/client/ftnames.o  : $(FTDIR)/ftnames.c;   $(DO_CC)

## Included by sfnt.c
# $(B)/client/ttload.o   : $(FTDIR)/ttload.c;    $(DO_CC)
# $(B)/client/ttcmap.o   : $(FTDIR)/ttcmap.c;    $(DO_CC)
# $(B)/client/sfobjs.o   : $(FTDIR)/sfobjs.c;    $(DO_CC)
# $(B)/client/sfdriver.o : $(FTDIR)/sfdriver.c;  $(DO_CC)
## Unused in sfnt.c
# $(B)/client/ttpost.o   : $(FTDIR)/ttpost.c;    $(DO_CC)
# $(B)/client/ttsbit.o   : $(FTDIR)/ttsbit.c;    $(DO_CC)

## Included by truetype.c
# $(B)/client/ttdriver.o : $(FTDIR)/ttdriver.c;  $(DO_CC)
# $(B)/client/ttgload.o  : $(FTDIR)/ttgload.c;   $(DO_CC)
# $(B)/client/ttobjs.o   : $(FTDIR)/ttobjs.c;    $(DO_CC)
# $(B)/client/ttpload.o  : $(FTDIR)/ttpload.c;   $(DO_CC)
## Unused in truetype.c
# $(B)/client/ttinterp.o : $(FTDIR)/ttinterp.c;  $(DO_CC)

## Included by smooth.c
# $(B)/client/ftsmooth.o : $(FTDIR)/ftsmooth.c;  $(DO_CC) -DFT_FLAT_COMPILE
# $(B)/client/ftgrays.o  : $(FTDIR)/ftgrays.c;   $(DO_CC)

#############################################################################
# DEDICATED SERVER                        
#############################################################################

Q3DOBJ = \
	$(B)/ded/sv_bot.o \
	$(B)/ded/sv_client.o \
	$(B)/ded/sv_ccmds.o \
	$(B)/ded/sv_game.o \
	$(B)/ded/sv_init.o \
	$(B)/ded/sv_main.o \
	$(B)/ded/sv_net_chan.o \
	$(B)/ded/sv_snapshot.o \
	$(B)/ded/sv_world.o \
	\
	$(B)/ded/cm_load.o \
	$(B)/ded/cm_patch.o \
	$(B)/ded/cm_polylib.o \
	$(B)/ded/cm_test.o \
	$(B)/ded/cm_trace.o \
	$(B)/ded/cmd.o \
	$(B)/ded/common.o \
	$(B)/ded/cvar.o \
	$(B)/ded/files.o \
	$(B)/ded/md4.o \
	$(B)/ded/msg.o \
	$(B)/ded/net_chan.o \
	$(B)/ded/huffman.o \
	\
	$(B)/ded/q_math.o \
	$(B)/ded/q_shared.o \
	\
	$(B)/ded/unzip.o \
	$(B)/ded/vm.o \
	$(B)/ded/vm_interpreted.o \
	\
	$(B)/ded/be_aas_bspq3.o \
	$(B)/ded/be_aas_cluster.o \
	$(B)/ded/be_aas_debug.o \
	$(B)/ded/be_aas_entity.o \
	$(B)/ded/be_aas_file.o \
	$(B)/ded/be_aas_main.o \
	$(B)/ded/be_aas_move.o \
	$(B)/ded/be_aas_optimize.o \
	$(B)/ded/be_aas_reach.o \
	$(B)/ded/be_aas_route.o \
	$(B)/ded/be_aas_routealt.o \
	$(B)/ded/be_aas_sample.o \
	$(B)/ded/be_ai_char.o \
	$(B)/ded/be_ai_chat.o \
	$(B)/ded/be_ai_gen.o \
	$(B)/ded/be_ai_goal.o \
	$(B)/ded/be_ai_move.o \
	$(B)/ded/be_ai_weap.o \
	$(B)/ded/be_ai_weight.o \
	$(B)/ded/be_ea.o \
	$(B)/ded/be_interface.o \
	$(B)/ded/l_crc.o \
	$(B)/ded/l_libvar.o \
	$(B)/ded/l_log.o \
	$(B)/ded/l_memory.o \
	$(B)/ded/l_precomp.o \
	$(B)/ded/l_script.o \
	$(B)/ded/l_struct.o \
	\
	$(B)/ded/linux_common.o \
	$(B)/ded/linux_signals.o \
	$(B)/ded/unix_main.o \
	$(B)/ded/unix_net.o \
	$(B)/ded/unix_shared.o \
	\
	$(B)/ded/null_client.o \
	$(B)/ded/null_input.o \
	$(B)/ded/null_snddma.o

ifeq ($(ARCH),i386)
  Q3DOBJ += $(B)/ded/vm_x86.o $(B)/ded/ftol.o $(B)/ded/snapvector.o
endif

ifeq ($(ARCH),ppc)
  ifeq ($(DLL_ONLY),false)
    Q3DOBJ += $(B)/ded/vm_ppc.o
  endif
endif

$(B)/$(PLATFORM)q3ded : $(Q3DOBJ)
	$(CC)  -o $@ $(Q3DOBJ) $(LDFLAGS)

$(B)/ded/sv_bot.o : $(SDIR)/sv_bot.c; $(DO_DED_CC) 
$(B)/ded/sv_client.o : $(SDIR)/sv_client.c; $(DO_DED_CC) 
$(B)/ded/sv_ccmds.o : $(SDIR)/sv_ccmds.c; $(DO_DED_CC) 
$(B)/ded/sv_game.o : $(SDIR)/sv_game.c; $(DO_DED_CC) 
$(B)/ded/sv_init.o : $(SDIR)/sv_init.c; $(DO_DED_CC) 
$(B)/ded/sv_main.o : $(SDIR)/sv_main.c; $(DO_DED_CC) 
$(B)/ded/sv_net_chan.o : $(SDIR)/sv_net_chan.c; $(DO_DED_CC) 
$(B)/ded/sv_snapshot.o : $(SDIR)/sv_snapshot.c; $(DO_DED_CC) 
$(B)/ded/sv_world.o : $(SDIR)/sv_world.c; $(DO_DED_CC) 
$(B)/ded/cm_load.o : $(CMDIR)/cm_load.c; $(DO_DED_CC) 
$(B)/ded/cm_polylib.o : $(CMDIR)/cm_polylib.c; $(DO_DED_CC) 
$(B)/ded/cm_test.o : $(CMDIR)/cm_test.c; $(DO_DED_CC) 
$(B)/ded/cm_trace.o : $(CMDIR)/cm_trace.c; $(DO_DED_CC) 
$(B)/ded/cm_patch.o : $(CMDIR)/cm_patch.c; $(DO_DED_CC) 
$(B)/ded/cmd.o : $(CMDIR)/cmd.c; $(DO_DED_CC) 
$(B)/ded/common.o : $(CMDIR)/common.c; $(DO_DED_CC) 
$(B)/ded/cvar.o : $(CMDIR)/cvar.c; $(DO_DED_CC) 
$(B)/ded/files.o : $(CMDIR)/files.c; $(DO_DED_CC) 
$(B)/ded/md4.o : $(CMDIR)/md4.c; $(DO_DED_CC) 
$(B)/ded/msg.o : $(CMDIR)/msg.c; $(DO_DED_CC) 
$(B)/ded/net_chan.o : $(CMDIR)/net_chan.c; $(DO_DED_CC)
$(B)/ded/huffman.o : $(CMDIR)/huffman.c; $(DO_DED_CC)
$(B)/ded/q_shared.o : $(GDIR)/q_shared.c; $(DO_DED_CC) 
$(B)/ded/q_math.o : $(GDIR)/q_math.c; $(DO_DED_CC) 

$(B)/ded/be_aas_bspq3.o : $(BLIBDIR)/be_aas_bspq3.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_cluster.o : $(BLIBDIR)/be_aas_cluster.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_debug.o : $(BLIBDIR)/be_aas_debug.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_entity.o : $(BLIBDIR)/be_aas_entity.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_file.o : $(BLIBDIR)/be_aas_file.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_main.o : $(BLIBDIR)/be_aas_main.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_move.o : $(BLIBDIR)/be_aas_move.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_optimize.o : $(BLIBDIR)/be_aas_optimize.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_reach.o : $(BLIBDIR)/be_aas_reach.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_route.o : $(BLIBDIR)/be_aas_route.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_routealt.o : $(BLIBDIR)/be_aas_routealt.c; $(DO_BOT_CC) 
$(B)/ded/be_aas_sample.o : $(BLIBDIR)/be_aas_sample.c; $(DO_BOT_CC) 
$(B)/ded/be_ai_char.o : $(BLIBDIR)/be_ai_char.c; $(DO_BOT_CC) 
$(B)/ded/be_ai_chat.o : $(BLIBDIR)/be_ai_chat.c; $(DO_BOT_CC) 
$(B)/ded/be_ai_gen.o : $(BLIBDIR)/be_ai_gen.c; $(DO_BOT_CC) 
$(B)/ded/be_ai_goal.o : $(BLIBDIR)/be_ai_goal.c; $(DO_BOT_CC) 
$(B)/ded/be_ai_move.o : $(BLIBDIR)/be_ai_move.c; $(DO_BOT_CC) 
$(B)/ded/be_ai_weap.o : $(BLIBDIR)/be_ai_weap.c; $(DO_BOT_CC) 
$(B)/ded/be_ai_weight.o : $(BLIBDIR)/be_ai_weight.c; $(DO_BOT_CC) 
$(B)/ded/be_ea.o : $(BLIBDIR)/be_ea.c; $(DO_BOT_CC) 
$(B)/ded/be_interface.o : $(BLIBDIR)/be_interface.c; $(DO_BOT_CC) 
$(B)/ded/l_crc.o : $(BLIBDIR)/l_crc.c; $(DO_BOT_CC) 
$(B)/ded/l_libvar.o : $(BLIBDIR)/l_libvar.c; $(DO_BOT_CC) 
$(B)/ded/l_log.o : $(BLIBDIR)/l_log.c; $(DO_BOT_CC) 
$(B)/ded/l_memory.o : $(BLIBDIR)/l_memory.c; $(DO_BOT_CC) 
$(B)/ded/l_precomp.o : $(BLIBDIR)/l_precomp.c; $(DO_BOT_CC) 
$(B)/ded/l_script.o : $(BLIBDIR)/l_script.c; $(DO_BOT_CC) 
$(B)/ded/l_struct.o : $(BLIBDIR)/l_struct.c; $(DO_BOT_CC) 

$(B)/ded/linux_common.o : $(UDIR)/linux_common.c; $(DO_CC)
$(B)/ded/linux_signals.o : $(UDIR)/linux_signals.c; $(DO_DED_CC)
$(B)/ded/unix_main.o : $(UDIR)/unix_main.c; $(DO_DED_CC) 
$(B)/ded/unix_net.o : $(UDIR)/unix_net.c; $(DO_DED_CC) 
$(B)/ded/unix_shared.o : $(UDIR)/unix_shared.c; $(DO_DED_CC) 
$(B)/ded/null_client.o : $(NDIR)/null_client.c; $(DO_DED_CC) 
$(B)/ded/null_input.o : $(NDIR)/null_input.c; $(DO_DED_CC) 
$(B)/ded/null_snddma.o : $(NDIR)/null_snddma.c; $(DO_DED_CC) 
$(B)/ded/unzip.o : $(CMDIR)/unzip.c; $(DO_DED_CC) 
$(B)/ded/vm.o : $(CMDIR)/vm.c; $(DO_DED_CC) 
$(B)/ded/vm_interpreted.o : $(CMDIR)/vm_interpreted.c; $(DO_DED_CC) 

ifeq ($(ARCH),i386)
$(B)/ded/vm_x86.o : $(CMDIR)/vm_x86.c; $(DO_DED_CC) 
$(B)/ded/ftol.o : $(UDIR)/ftol.nasm; $(DO_NASM)
$(B)/ded/snapvector.o : $(UDIR)/snapvector.nasm; $(DO_NASM) 
endif

ifeq ($(ARCH),ppc)
ifeq ($(DLL_ONLY),false)
$(B)/ded/vm_ppc.o : $(CMDIR)/vm_ppc.c; $(DO_DED_CC)
endif
endif


#############################################################################
## BASEQ3 CGAME
#############################################################################

Q3CGOBJ = \
	$(B)/baseq3/cgame/bg_misc.o \
	$(B)/baseq3/cgame/bg_pmove.o \
	$(B)/baseq3/cgame/bg_slidemove.o \
	$(B)/baseq3/cgame/cg_consolecmds.o \
	$(B)/baseq3/cgame/cg_draw.o \
	$(B)/baseq3/cgame/cg_drawtools.o \
	$(B)/baseq3/cgame/cg_effects.o \
	$(B)/baseq3/cgame/cg_ents.o \
	$(B)/baseq3/cgame/cg_event.o \
	$(B)/baseq3/cgame/cg_info.o \
	$(B)/baseq3/cgame/cg_localents.o \
	$(B)/baseq3/cgame/cg_main.o \
	$(B)/baseq3/cgame/cg_marks.o \
	$(B)/baseq3/cgame/cg_players.o \
	$(B)/baseq3/cgame/cg_playerstate.o \
	$(B)/baseq3/cgame/cg_predict.o \
	$(B)/baseq3/cgame/cg_scoreboard.o \
	$(B)/baseq3/cgame/cg_servercmds.o \
	$(B)/baseq3/cgame/cg_snapshot.o \
	$(B)/baseq3/cgame/cg_syscalls.o \
	$(B)/baseq3/cgame/cg_view.o \
	$(B)/baseq3/cgame/cg_weapons.o \
	$(B)/baseq3/cgame/q_math.o \
	$(B)/baseq3/cgame/q_shared.o

$(B)/baseq3/cgame$(ARCH).$(SHLIBEXT) : $(Q3CGOBJ)
	$(CC)  $(SHLIBLDFLAGS) -o $@ $(Q3CGOBJ)

$(B)/baseq3/cgame/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_SHLIB_CC)         
$(B)/baseq3/cgame/bg_pmove.o : $(GDIR)/bg_pmove.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/bg_slidemove.o : $(GDIR)/bg_slidemove.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_consolecmds.o : $(CGDIR)/cg_consolecmds.c; $(DO_SHLIB_CC)       
$(B)/baseq3/cgame/cg_draw.o : $(CGDIR)/cg_draw.c; $(DO_SHLIB_CC)             
$(B)/baseq3/cgame/cg_drawtools.o : $(CGDIR)/cg_drawtools.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_effects.o : $(CGDIR)/cg_effects.c; $(DO_SHLIB_CC)           
$(B)/baseq3/cgame/cg_ents.o : $(CGDIR)/cg_ents.c; $(DO_SHLIB_CC)               
$(B)/baseq3/cgame/cg_event.o : $(CGDIR)/cg_event.c; $(DO_SHLIB_CC)            
$(B)/baseq3/cgame/cg_info.o : $(CGDIR)/cg_info.c; $(DO_SHLIB_CC)               
$(B)/baseq3/cgame/cg_localents.o : $(CGDIR)/cg_localents.c; $(DO_SHLIB_CC)           
$(B)/baseq3/cgame/cg_main.o : $(CGDIR)/cg_main.c; $(DO_SHLIB_CC)                   
$(B)/baseq3/cgame/cg_marks.o : $(CGDIR)/cg_marks.c; $(DO_SHLIB_CC)                 
$(B)/baseq3/cgame/cg_players.o : $(CGDIR)/cg_players.c; $(DO_SHLIB_CC)             
$(B)/baseq3/cgame/cg_playerstate.o : $(CGDIR)/cg_playerstate.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_predict.o : $(CGDIR)/cg_predict.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_scoreboard.o : $(CGDIR)/cg_scoreboard.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_servercmds.o : $(CGDIR)/cg_servercmds.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_snapshot.o : $(CGDIR)/cg_snapshot.c; $(DO_SHLIB_CC)             
$(B)/baseq3/cgame/cg_syscalls.o : $(CGDIR)/cg_syscalls.c; $(DO_SHLIB_CC)        
$(B)/baseq3/cgame/cg_view.o : $(CGDIR)/cg_view.c; $(DO_SHLIB_CC)                     
$(B)/baseq3/cgame/cg_weapons.o : $(CGDIR)/cg_weapons.c; $(DO_SHLIB_CC)               
$(B)/baseq3/cgame/q_math.o : $(GDIR)/q_math.c; $(DO_SHLIB_CC)                       
$(B)/baseq3/cgame/q_shared.o : $(GDIR)/q_shared.c; $(DO_SHLIB_CC)                     

#############################################################################
## MISSIONPACK CGAME
#############################################################################

MPCGOBJ = \
	$(B)/missionpack/cgame/bg_misc.o \
	$(B)/missionpack/cgame/bg_pmove.o \
	$(B)/missionpack/cgame/bg_slidemove.o \
	$(B)/missionpack/cgame/cg_consolecmds.o \
	$(B)/missionpack/cgame/cg_newdraw.o \
	$(B)/missionpack/cgame/cg_draw.o \
	$(B)/missionpack/cgame/cg_drawtools.o \
	$(B)/missionpack/cgame/cg_effects.o \
	$(B)/missionpack/cgame/cg_ents.o \
	$(B)/missionpack/cgame/cg_event.o \
	$(B)/missionpack/cgame/cg_info.o \
	$(B)/missionpack/cgame/cg_localents.o \
	$(B)/missionpack/cgame/cg_main.o \
	$(B)/missionpack/cgame/cg_marks.o \
	$(B)/missionpack/cgame/cg_players.o \
	$(B)/missionpack/cgame/cg_playerstate.o \
	$(B)/missionpack/cgame/cg_predict.o \
	$(B)/missionpack/cgame/cg_scoreboard.o \
	$(B)/missionpack/cgame/cg_servercmds.o \
	$(B)/missionpack/cgame/cg_snapshot.o \
	$(B)/missionpack/cgame/cg_syscalls.o \
	$(B)/missionpack/cgame/cg_view.o \
	$(B)/missionpack/cgame/cg_weapons.o \
	$(B)/missionpack/cgame/q_math.o \
	$(B)/missionpack/cgame/q_shared.o \
	$(B)/missionpack/cgame/ui_shared.o

$(B)/missionpack/cgame$(ARCH).$(SHLIBEXT) : $(MPCGOBJ)
	$(CC) $(SHLIBLDFLAGS) -o $@ $(MPCGOBJ)

$(B)/missionpack/cgame/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_SHLIB_CC)     -DMISSIONPACK 
$(B)/missionpack/cgame/bg_pmove.o : $(GDIR)/bg_pmove.c; $(DO_SHLIB_CC)    -DMISSIONPACK 
$(B)/missionpack/cgame/bg_slidemove.o : $(GDIR)/bg_slidemove.c; $(DO_SHLIB_CC)   -DMISSIONPACK 
$(B)/missionpack/cgame/cg_consolecmds.o : $(CGDIR)/cg_consolecmds.c; $(DO_SHLIB_CC)   -DMISSIONPACK 
$(B)/missionpack/cgame/cg_newdraw.o : $(CGDIR)/cg_newdraw.c; $(DO_SHLIB_CC)         -DMISSIONPACK
$(B)/missionpack/cgame/cg_draw.o : $(CGDIR)/cg_draw.c; $(DO_SHLIB_CC)    -DMISSIONPACK  
$(B)/missionpack/cgame/cg_drawtools.o : $(CGDIR)/cg_drawtools.c; $(DO_SHLIB_CC)    -DMISSIONPACK 
$(B)/missionpack/cgame/cg_effects.o : $(CGDIR)/cg_effects.c; $(DO_SHLIB_CC)       -DMISSIONPACK 
$(B)/missionpack/cgame/cg_ents.o : $(CGDIR)/cg_ents.c; $(DO_SHLIB_CC)           -DMISSIONPACK 
$(B)/missionpack/cgame/cg_event.o : $(CGDIR)/cg_event.c; $(DO_SHLIB_CC)        -DMISSIONPACK 
$(B)/missionpack/cgame/cg_info.o : $(CGDIR)/cg_info.c; $(DO_SHLIB_CC)           -DMISSIONPACK 
$(B)/missionpack/cgame/cg_localents.o : $(CGDIR)/cg_localents.c; $(DO_SHLIB_CC)   -DMISSIONPACK 
$(B)/missionpack/cgame/cg_main.o : $(CGDIR)/cg_main.c; $(DO_SHLIB_CC)             -DMISSIONPACK 
$(B)/missionpack/cgame/cg_marks.o : $(CGDIR)/cg_marks.c; $(DO_SHLIB_CC)          -DMISSIONPACK  
$(B)/missionpack/cgame/cg_players.o : $(CGDIR)/cg_players.c; $(DO_SHLIB_CC)       -DMISSIONPACK 
$(B)/missionpack/cgame/cg_playerstate.o : $(CGDIR)/cg_playerstate.c; $(DO_SHLIB_CC)  -DMISSIONPACK 
$(B)/missionpack/cgame/cg_predict.o : $(CGDIR)/cg_predict.c; $(DO_SHLIB_CC)  -DMISSIONPACK 
$(B)/missionpack/cgame/cg_scoreboard.o : $(CGDIR)/cg_scoreboard.c; $(DO_SHLIB_CC)  -DMISSIONPACK 
$(B)/missionpack/cgame/cg_servercmds.o : $(CGDIR)/cg_servercmds.c; $(DO_SHLIB_CC)  -DMISSIONPACK 
$(B)/missionpack/cgame/cg_snapshot.o : $(CGDIR)/cg_snapshot.c; $(DO_SHLIB_CC)       -DMISSIONPACK 
$(B)/missionpack/cgame/cg_syscalls.o : $(CGDIR)/cg_syscalls.c; $(DO_SHLIB_CC)  -DMISSIONPACK 
$(B)/missionpack/cgame/cg_view.o : $(CGDIR)/cg_view.c; $(DO_SHLIB_CC)               -DMISSIONPACK 
$(B)/missionpack/cgame/cg_weapons.o : $(CGDIR)/cg_weapons.c; $(DO_SHLIB_CC)        -DMISSIONPACK  
$(B)/missionpack/cgame/q_math.o : $(GDIR)/q_math.c; $(DO_SHLIB_CC)                 -DMISSIONPACK 
$(B)/missionpack/cgame/q_shared.o : $(GDIR)/q_shared.c; $(DO_SHLIB_CC)             -DMISSIONPACK 
$(B)/missionpack/cgame/ui_shared.o : $(UIDIR)/ui_shared.c; $(DO_SHLIB_CC)             -DMISSIONPACK 



#############################################################################
## BASEQ3 GAME
#############################################################################

Q3GOBJ = \
	$(B)/baseq3/game/ai_chat.o \
	$(B)/baseq3/game/ai_cmd.o \
	$(B)/baseq3/game/ai_dmnet.o \
	$(B)/baseq3/game/ai_dmq3.o \
	$(B)/baseq3/game/ai_main.o \
	$(B)/baseq3/game/ai_team.o \
	$(B)/baseq3/game/ai_vcmd.o \
	$(B)/baseq3/game/bg_misc.o \
	$(B)/baseq3/game/bg_pmove.o \
	$(B)/baseq3/game/bg_slidemove.o \
	$(B)/baseq3/game/g_active.o \
	$(B)/baseq3/game/g_arenas.o \
	$(B)/baseq3/game/g_bot.o \
	$(B)/baseq3/game/g_client.o \
	$(B)/baseq3/game/g_cmds.o \
	$(B)/baseq3/game/g_combat.o \
	$(B)/baseq3/game/g_items.o \
	$(B)/baseq3/game/g_main.o \
	$(B)/baseq3/game/g_mem.o \
	$(B)/baseq3/game/g_misc.o \
	$(B)/baseq3/game/g_missile.o \
	$(B)/baseq3/game/g_mover.o \
	$(B)/baseq3/game/g_session.o \
	$(B)/baseq3/game/g_spawn.o \
	$(B)/baseq3/game/g_svcmds.o \
	$(B)/baseq3/game/g_syscalls.o \
	$(B)/baseq3/game/g_target.o \
	$(B)/baseq3/game/g_team.o \
	$(B)/baseq3/game/g_trigger.o \
	$(B)/baseq3/game/g_utils.o \
	$(B)/baseq3/game/g_weapon.o \
	\
	$(B)/baseq3/game/q_math.o \
	$(B)/baseq3/game/q_shared.o

$(B)/baseq3/qagame$(ARCH).$(SHLIBEXT) : $(Q3GOBJ)
	$(CC)  $(SHLIBLDFLAGS) -o $@ $(Q3GOBJ)

$(B)/baseq3/game/ai_chat.o : $(GDIR)/ai_chat.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/ai_cmd.o : $(GDIR)/ai_cmd.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/ai_dmnet.o : $(GDIR)/ai_dmnet.c; $(DO_SHLIB_CC)   
$(B)/baseq3/game/ai_dmq3.o : $(GDIR)/ai_dmq3.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/ai_main.o : $(GDIR)/ai_main.c; $(DO_SHLIB_CC)      
$(B)/baseq3/game/ai_team.o : $(GDIR)/ai_team.c; $(DO_SHLIB_CC)  
$(B)/baseq3/game/ai_vcmd.o : $(GDIR)/ai_vcmd.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/bg_pmove.o : $(GDIR)/bg_pmove.c; $(DO_SHLIB_CC)     
$(B)/baseq3/game/bg_slidemove.o : $(GDIR)/bg_slidemove.c; $(DO_SHLIB_CC)    
$(B)/baseq3/game/g_active.o : $(GDIR)/g_active.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_arenas.o : $(GDIR)/g_arenas.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_bot.o : $(GDIR)/g_bot.c; $(DO_SHLIB_CC)            
$(B)/baseq3/game/g_client.o : $(GDIR)/g_client.c; $(DO_SHLIB_CC)      
$(B)/baseq3/game/g_cmds.o : $(GDIR)/g_cmds.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_combat.o : $(GDIR)/g_combat.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_items.o : $(GDIR)/g_items.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_main.o : $(GDIR)/g_main.c; $(DO_SHLIB_CC)            
$(B)/baseq3/game/g_mem.o : $(GDIR)/g_mem.c; $(DO_SHLIB_CC)               
$(B)/baseq3/game/g_misc.o : $(GDIR)/g_misc.c; $(DO_SHLIB_CC)             
$(B)/baseq3/game/g_missile.o : $(GDIR)/g_missile.c; $(DO_SHLIB_CC)        
$(B)/baseq3/game/g_mover.o : $(GDIR)/g_mover.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_session.o : $(GDIR)/g_session.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_spawn.o : $(GDIR)/g_spawn.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_svcmds.o : $(GDIR)/g_svcmds.c; $(DO_SHLIB_CC)         
$(B)/baseq3/game/g_syscalls.o : $(GDIR)/g_syscalls.c; $(DO_SHLIB_CC)    
$(B)/baseq3/game/g_target.o : $(GDIR)/g_target.c; $(DO_SHLIB_CC)        
$(B)/baseq3/game/g_team.o : $(GDIR)/g_team.c; $(DO_SHLIB_CC)           
$(B)/baseq3/game/g_trigger.o : $(GDIR)/g_trigger.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/g_utils.o : $(GDIR)/g_utils.c; $(DO_SHLIB_CC)          
$(B)/baseq3/game/g_weapon.o : $(GDIR)/g_weapon.c; $(DO_SHLIB_CC)       
$(B)/baseq3/game/q_math.o : $(GDIR)/q_math.c; $(DO_SHLIB_CC)            
$(B)/baseq3/game/q_shared.o : $(GDIR)/q_shared.c; $(DO_SHLIB_CC)       


#############################################################################
## MISSIONPACK GAME
#############################################################################

MPGOBJ = \
	$(B)/missionpack/game/ai_chat.o \
	$(B)/missionpack/game/ai_cmd.o \
	$(B)/missionpack/game/ai_dmnet.o \
	$(B)/missionpack/game/ai_dmq3.o \
	$(B)/missionpack/game/ai_main.o \
	$(B)/missionpack/game/ai_team.o \
	$(B)/missionpack/game/ai_vcmd.o \
	$(B)/missionpack/game/bg_misc.o \
	$(B)/missionpack/game/bg_pmove.o \
	$(B)/missionpack/game/bg_slidemove.o \
	$(B)/missionpack/game/g_active.o \
	$(B)/missionpack/game/g_arenas.o \
	$(B)/missionpack/game/g_bot.o \
	$(B)/missionpack/game/g_client.o \
	$(B)/missionpack/game/g_cmds.o \
	$(B)/missionpack/game/g_combat.o \
	$(B)/missionpack/game/g_items.o \
	$(B)/missionpack/game/g_main.o \
	$(B)/missionpack/game/g_mem.o \
	$(B)/missionpack/game/g_misc.o \
	$(B)/missionpack/game/g_missile.o \
	$(B)/missionpack/game/g_mover.o \
	$(B)/missionpack/game/g_session.o \
	$(B)/missionpack/game/g_spawn.o \
	$(B)/missionpack/game/g_svcmds.o \
	$(B)/missionpack/game/g_syscalls.o \
	$(B)/missionpack/game/g_target.o \
	$(B)/missionpack/game/g_team.o \
	$(B)/missionpack/game/g_trigger.o \
	$(B)/missionpack/game/g_utils.o \
	$(B)/missionpack/game/g_weapon.o \
	\
	$(B)/missionpack/game/q_math.o \
	$(B)/missionpack/game/q_shared.o

$(B)/missionpack/qagame$(ARCH).$(SHLIBEXT) : $(MPGOBJ)
	$(CC) $(SHLIBLDFLAGS) -o $@ $(MPGOBJ)

$(B)/missionpack/game/ai_chat.o : $(GDIR)/ai_chat.c; $(DO_SHLIB_CC)    -DMISSIONPACK 
$(B)/missionpack/game/ai_cmd.o : $(GDIR)/ai_cmd.c; $(DO_SHLIB_CC)      -DMISSIONPACK 
$(B)/missionpack/game/ai_dmnet.o : $(GDIR)/ai_dmnet.c; $(DO_SHLIB_CC)  -DMISSIONPACK 
$(B)/missionpack/game/ai_dmq3.o : $(GDIR)/ai_dmq3.c; $(DO_SHLIB_CC)    -DMISSIONPACK   
$(B)/missionpack/game/ai_main.o : $(GDIR)/ai_main.c; $(DO_SHLIB_CC)    -DMISSIONPACK  
$(B)/missionpack/game/ai_team.o : $(GDIR)/ai_team.c; $(DO_SHLIB_CC)    -DMISSIONPACK 
$(B)/missionpack/game/ai_vcmd.o : $(GDIR)/ai_vcmd.c; $(DO_SHLIB_CC)    -DMISSIONPACK 
$(B)/missionpack/game/bg_misc.o : $(GDIR)/bg_misc.c; $(DO_SHLIB_CC)    -DMISSIONPACK 
$(B)/missionpack/game/bg_pmove.o : $(GDIR)/bg_pmove.c; $(DO_SHLIB_CC)    -DMISSIONPACK 
$(B)/missionpack/game/bg_slidemove.o : $(GDIR)/bg_slidemove.c; $(DO_SHLIB_CC)  -DMISSIONPACK  
$(B)/missionpack/game/g_active.o : $(GDIR)/g_active.c; $(DO_SHLIB_CC)      -DMISSIONPACK 
$(B)/missionpack/game/g_arenas.o : $(GDIR)/g_arenas.c; $(DO_SHLIB_CC)      -DMISSIONPACK 
$(B)/missionpack/game/g_bot.o : $(GDIR)/g_bot.c; $(DO_SHLIB_CC)           -DMISSIONPACK 
$(B)/missionpack/game/g_client.o : $(GDIR)/g_client.c; $(DO_SHLIB_CC)     -DMISSIONPACK 
$(B)/missionpack/game/g_cmds.o : $(GDIR)/g_cmds.c; $(DO_SHLIB_CC)         -DMISSIONPACK 
$(B)/missionpack/game/g_combat.o : $(GDIR)/g_combat.c; $(DO_SHLIB_CC)      -DMISSIONPACK 
$(B)/missionpack/game/g_items.o : $(GDIR)/g_items.c; $(DO_SHLIB_CC)         -DMISSIONPACK 
$(B)/missionpack/game/g_main.o : $(GDIR)/g_main.c; $(DO_SHLIB_CC)          -DMISSIONPACK  
$(B)/missionpack/game/g_mem.o : $(GDIR)/g_mem.c; $(DO_SHLIB_CC)             -DMISSIONPACK  
$(B)/missionpack/game/g_misc.o : $(GDIR)/g_misc.c; $(DO_SHLIB_CC)            -DMISSIONPACK 
$(B)/missionpack/game/g_missile.o : $(GDIR)/g_missile.c; $(DO_SHLIB_CC)      -DMISSIONPACK  
$(B)/missionpack/game/g_mover.o : $(GDIR)/g_mover.c; $(DO_SHLIB_CC)        -DMISSIONPACK  
$(B)/missionpack/game/g_session.o : $(GDIR)/g_session.c; $(DO_SHLIB_CC)     -DMISSIONPACK  
$(B)/missionpack/game/g_spawn.o : $(GDIR)/g_spawn.c; $(DO_SHLIB_CC)         -DMISSIONPACK 
$(B)/missionpack/game/g_svcmds.o : $(GDIR)/g_svcmds.c; $(DO_SHLIB_CC)       -DMISSIONPACK  
$(B)/missionpack/game/g_syscalls.o : $(GDIR)/g_syscalls.c; $(DO_SHLIB_CC)   -DMISSIONPACK 
$(B)/missionpack/game/g_target.o : $(GDIR)/g_target.c; $(DO_SHLIB_CC)       -DMISSIONPACK 
$(B)/missionpack/game/g_team.o : $(GDIR)/g_team.c; $(DO_SHLIB_CC)         -DMISSIONPACK  
$(B)/missionpack/game/g_trigger.o : $(GDIR)/g_trigger.c; $(DO_SHLIB_CC)    -DMISSIONPACK   
$(B)/missionpack/game/g_utils.o : $(GDIR)/g_utils.c; $(DO_SHLIB_CC)        -DMISSIONPACK  
$(B)/missionpack/game/g_weapon.o : $(GDIR)/g_weapon.c; $(DO_SHLIB_CC)      -DMISSIONPACK 
$(B)/missionpack/game/q_math.o : $(GDIR)/q_math.c; $(DO_SHLIB_CC)          -DMISSIONPACK  
$(B)/missionpack/game/q_shared.o : $(GDIR)/q_shared.c; $(DO_SHLIB_CC)      -DMISSIONPACK 
