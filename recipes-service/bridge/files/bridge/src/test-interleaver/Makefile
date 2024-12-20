#
# Top level makefile for the current project
#
# Usage:
# 	make
#   make clean

# source root directory where all components and project directory reside
ifndef PROJECT_SOURCE_ROOT
PROJECT_SOURCE_ROOT = ..
endif
SOURCE_ROOT = $(PROJECT_SOURCE_ROOT)

# build directory; use current directory unless specified otherwise
ifndef BUILDDIR
BUILDDIR = $(shell pwd)
endif

# installation directory
ifndef INSTALLDIR
INSTALLDIR = ./
endif

# external libraries
LIBS = " -lm -lturbofec "

# project
PROJECT = test-interleaver

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

