DESCRIPTION = "Recipe to build and install the wifi service "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

# project name
prj="wifi"

inherit systemd

# Create dependency to library packages
RDEPENDS:${PN} += " wifi-reset "

#src uri's for the source code and .service conf file
SRC_URI +="file://${prj}.service"
SRC_URI +="file://logrotate.d"
SRC_URI +="file://LICENSE"
SRC_URI +="file://${prj}"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

service_dir = "/etc/systemd/system"

#add dir to the package files variable
FILES:${PN} += "${service_dir}/* "
FILES:${PN} += "/home/wifi/*"

do_install(){ 
	cd ${WORKDIR}
	find ${prj} -type f -exec install -Dm 644 "{}" "${D}/home/{}" \;
	
	#install the service file to /etc/systemd/system
	install -Dm 644 ${prj}.service ${D}${service_dir}/${prj}.service
	
        #install -Dm 644 ${prj}.py ${D}/root/${prj}.py
        
	#install the logrotate configuration file
	install -Dm 644 logrotate.d/${prj} ${D}/etc/logrotate.d/${prj}

	cd -
        
        cd ${D}/home/${prj}/
        chmod +x *.sh
        cd -
	
}

