# Recipe created by recipetool
# This is the basis of a recipe and may need further editing in order to be fully functional.
# (Feel free to remove these comments when editing.)

# WARNING: the following LICENSE and LIC_FILES_CHKSUM values are best guesses - it is
# your responsibility to verify that the values are complete and correct.
#
# The following license files were not able to be identified and are
# represented as "Unknown" below, you will need to check them yourself:
#   LICENSE
#LICENSE = "Unknown"
LICENSE = "GPLv3"
LIC_FILES_CHKSUM = "file://LICENSE;md5=51b35d652c070d136bf20244494be2d3"

SRC_URI = "git://github.com/ttsou/turbofec.git;protocol=https;branch=master \
           file://0001-added-extern-to-codes.h.patch \
           file://0002-modified-march-armv7.patch \
           file://0001-added-mpfu-neon-to-Makefile.patch \
           file://0002-added-sse2neon.h.patch \
           file://0003-included-sse2neon.h-removed-includes-for-mmintrin.h.patch \
           file://0004-modified-conv_gen.h.patch \
           "

# Modify these as desired
PV = "0.1+git${SRCPV}"
SRCREV = "6de1f4604933d6c21a0ff0c75401cffa7debf3cd"

S = "${WORKDIR}/git"

# NOTE: if this software is not capable of being built in a separate build directory
# from the source, you should replace autotools with autotools-brokensep in the
# inherit line
inherit autotools

# Specify any options you want to pass to the configure script using EXTRA_OECONF:
EXTRA_OECONF = " "
#EXTRA_OEMAKE = " -march=armv7 "

