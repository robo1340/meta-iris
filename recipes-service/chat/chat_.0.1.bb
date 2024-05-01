DESCRIPTION = "Recipe to build and install the chat service "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

# project name
prj="chat"

inherit systemd

# Create dependency to library packages
RDEPENDS:${PN} += " bridge "

#src uri's for the source code and .service conf file
SRC_URI +="file://${prj}.service"
SRC_URI +="file://logrotate.d"
SRC_URI +="file://LICENSE"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

service_dir = "/etc/systemd/system"

#add dir to the package files variable
#FILES:${PN} += "${service_dir}/* "
#FILES:${PN} += "/${prj}"

do_install(){ 
	cd ${WORKDIR}
	
	#install the service file to /etc/systemd/system
	install -Dm 644 ${prj}.service ${D}${service_dir}/${prj}.service

	#install the logrotate configuration file
	install -Dm 644 logrotate.d/${prj} ${D}/etc/logrotate.d/${prj}

	cd -
	
}

