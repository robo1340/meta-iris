#
# Top level makefile for SERA
#
# Usage:
# 	make
# 

# source root directory where all components and project directory reside
ifndef ARES_SOURCE_ROOT
ARES_SOURCE_ROOT = ..
endif
SOURCE_ROOT = $(ARES_SOURCE_ROOT)

# build directory; use current directory unless specified otherwise
#ifndef BUILDDIR
BUILDDIR = $(shell pwd)
#endif

# installation directory
#ifndef INSTALLDIR
#INSTALLDIR = /ares/bin
INSTALLDIR = ${WORKDIR}
#endif

# external libraries
LIBS = "-lzmq -lczmq "

# project
PROJECT = prj-zmq-send

all:
	$(MAKE) -f $(SOURCE_ROOT)/build-utils/makefile-project \
	PROJECT=$(PROJECT)              \
	LIBS=$(LIBS)										\
	SOURCE_ROOT=$(SOURCE_ROOT)      \
	BUILDDIR=$(BUILDDIR)            \
	INSTALLDIR=$(INSTALLDIR)

clean:
	$(MAKE) -f $(SOURCE_ROOT)/build-utils/makefile-project clean \
	PROJECT=$(PROJECT)              \
	LIBS=$(LIBS)										\
	SOURCE_ROOT=$(SOURCE_ROOT)      \
	BUILDDIR=$(BUILDDIR)            \
	INSTALLDIR=$(INSTALLDIR)

.PHONY : all clean

