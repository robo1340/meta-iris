DESCRIPTION = "Recipe to build and install the slip ip radio bridge "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

# project name
prj="bridge"

#inherit systemd

# Create dependency to library packages
RDEPENDS:${PN} += " zeromq zeromq-dev czmq libczmq-dev "
RDEPENDS:${PN} += " libgpiod libgpiod-dev libserialport "
RDEPENDS:${PN} += " turbofec net-tools socat "
DEPENDS = " zeromq czmq libgpiod turbofec libserialport net-tools socat "

#src uri's for the source code and .service conf file
SRC_URI +="file://bridge"
#SRC_URI +="file://${prj}.service"
#SRC_URI +="file://logrotate.d"
SRC_URI +="file://LICENSE"

#SYSTEMD_AUTO_ENABLE:${PN} = "enable"
#SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

do_compile(){

	#cross-compile proxy binary	
	cd ${WORKDIR}/${prj}/src/prj-${prj}
	#cd ${S}/${prj}/src/prj-${prj}
	make clean
	make	
	cd -

}

service_dir = "/etc/systemd/system"

#add dir to the package files variable
#FILES:${PN} += "${service_dir}/* "
FILES:${PN} += "/${prj}"

do_install(){
 
  #navigate to the working directory where everything was compiled and install
  #all the files and subdirectories in the proxy directory  
	cd ${WORKDIR}

	find ${prj} -type f -exec install -Dm 644 "{}" "${D}/{}" \;
	
	#install the service file to /etc/systemd/system
	#install -Dm 644 ${prj}.service ${D}${service_dir}/${prj}.service

	#install the logrotate configuration file
	#install -Dm 644 logrotate.d/${prj} ${D}/etc/logrotate.d/${prj}

	#install the compiled binary
        install -Dm 755 ${prj}/src/prj-${prj}/test ${D}/${prj}/src/prj-${prj}/bridge	
	cd -
	
}

