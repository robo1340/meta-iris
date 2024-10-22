DESCRIPTION = "Recipe to build and install the nodejs web service "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

RM_WORK_EXCLUDE += "${PN}"

# project name
prj="node"

inherit systemd

# Create dependency to library packages
RDEPENDS:${PN} += " bridge tiles "
PACKAGES="${PN}"

#src uri's for the source code and .service conf file
SRC_URI +="file://${prj}.service"
SRC_URI +="file://logrotate.d"
SRC_URI +="file://LICENSE"
SRC_URI +="file://nodejs"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

service_dir = "/etc/systemd/system"

#add dir to the package files variable
FILES:${PN} += "${service_dir}/${prj}.service "
FILES:${PN} += "/home/nodejs/*"

do_install(){ 
	cd ${WORKDIR} 
        find nodejs -type f -exec install -Dm 644 "{}" "${D}/home/{}" \;	
        cd -
 
        cd ${D}/home/nodejs 
        ln -s ../tiles tiles
        cd -
 
        cd ${WORKDIR}

	
        #install the service file to /etc/systemd/system
	install -Dm 644 ${prj}.service ${D}${service_dir}/${prj}.service

	#install the logrotate configuration file
	install -Dm 644 logrotate.d/${prj} ${D}/etc/logrotate.d/${prj}

	cd -
	
}

