#ARM_GCC = "gcc-arm-none-eabi-9-2020-q2-update"
#ARM_GCC_POSTFIX = "-aarch64-linux"
#ROOT_BIN = "${ROOT_HOME}/bin"
FILESEXTRAPATHS:append := "${THISDIR}/files:"
SRC_URI += " file://dot.vimrc "
SRC_URI += " file://dot.bashrc "
SRC_URI += " file://dot.profile "
#						 file://${ARM_GCC}${ARM_GCC_POSTFIX}.tar.bz2"
FILES_${PN} += " ${ROOT_HOME}/dot.vimrc " 
#FILES_${PN} += " ${ROOT_HOME}/dot.bashrc " 
#FILES_${PN} += " ${ROOT_HOME}/dot.profile " 

do_install:append() {

	#create mountpoints for the boot flash module
	#install -m 0755 -d ${D}/mnt/boot

	# custom .vimrc
  	install -m 0755 ${WORKDIR}/dot.vimrc ${D}${ROOT_HOME}/.vimrc

# A directory for M4 toolchain and other prebuilt development binaries
# install -m 0755 -d ${D}${ROOT_BIN}
# Pre-compiled M4 toolchain (gcc)
#	cp -R --no-preserve=ownership --no-dereference --preserve=mode,timestamps,links ${WORKDIR}/${ARM_GCC} ${D}${ROOT_BIN}
}

