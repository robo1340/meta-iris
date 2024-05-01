
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " file://dot.vimrc "
SRC_URI += " file://dot.bashrc "

FILES:${PN} += " ${ROOT_HOME}/dot.vimrc " 
FILES:${PN} += " ${ROOT_HOME}/dot.bashrc " 

do_install:append() {

	# custom .vimrc
  	install -m 0755 ${WORKDIR}/dot.vimrc ${D}${ROOT_HOME}/.vimrc
	# custom bashrc
  	install -m 0755 ${WORKDIR}/dot.bashrc ${D}${ROOT_HOME}/.bashrc
	# custom profile sources bashrc if it exists
}

