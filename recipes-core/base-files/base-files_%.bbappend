
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI += " file://dot.vimrc "
SRC_URI += " file://dot.bashrc "
SRC_URI += " file://src.sh "

FILES:${PN} += " ${ROOT_HOME}/dot.vimrc " 
FILES:${PN} += " ${ROOT_HOME}/dot.bashrc " 
FILES:${PN} += " /etc/profile.d/src.sh " 

do_install:append() {

	# custom .vimrc
  	install -m 0755 ${WORKDIR}/dot.vimrc ${D}${ROOT_HOME}/.vimrc
	# custom bashrc
  	install -m 0755 ${WORKDIR}/dot.bashrc ${D}${ROOT_HOME}/.bashrc
	# custom profile sources bashrc if it exists	
        install -Dm 0755 ${WORKDIR}/src.sh ${D}/etc/profile.d/src.sh
}

