DESCRIPTION = "Recipe to build and install the web key logger service "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

RM_WORK_EXCLUDE += "${PN}"

# project name
prj="web_key_logger"

inherit systemd

# Create dependency to library packages
RDEPENDS:${PN} += " hub "
PACKAGES="${PN}"

#src uri's for the source code and .service conf file
SRC_URI +="file://${prj}.service"
SRC_URI +="file://logrotate.d"
SRC_URI +="file://LICENSE"
SRC_URI +="file://${prj}"

SYSTEMD_AUTO_ENABLE:${PN} = "disable"
SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

service_dir = "/etc/systemd/system"

#add dir to the package files variable
FILES:${PN} += "${service_dir}/${prj}.service "
FILES:${PN} += "/home/${prj}/*"

do_install(){ 
	cd ${WORKDIR} 
        find ${prj} -type f -exec install -Dm 644 "{}" "${D}/home/{}" \;	
	
        #install the service file to /etc/systemd/system
	install -Dm 644 ${prj}.service ${D}${service_dir}/${prj}.service

	#install the logrotate configuration file
	install -Dm 644 logrotate.d/${prj} ${D}/etc/logrotate.d/${prj}

	cd -
	
}

