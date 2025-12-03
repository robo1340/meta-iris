DESCRIPTION = "Recipe to build and install the snap radio program "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"
RM_WORK_EXCLUDE += "${PN}"
# project name
prj="hub"

inherit systemd

# Create dependency to library packages
RDEPENDS:${PN} += " python3-pyzmq "
RDEPENDS:${PN} += " snap  "
DEPENDS = " "

#src uri's for the source code and .service conf file
SRC_URI +="file://${prj}"
SRC_URI +="file://${prj}.service"
SRC_URI +="file://logrotate.d"
SRC_URI +="file://LICENSE"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"
PACKAGES = "${PN}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

INSANE_SKIP:${PN}:append = "already-stripped"

service_dir = "/etc/systemd/system"

#add dir to the package files variable
#FILES:${PN} += "${service_dir}/* "
FILES:${PN} += "/${prj}"

do_install(){
 
  #navigate to the working directory where everything was compiled and install
  #all the files and subdirectories in the proxy directory  
	cd ${WORKDIR}
	find ${prj} -type f -exec install -Dm 644 "{}" "${D}/{}" \;
       
	install -d ${D}/${prj}/volatile

        rm ${D}/${prj}/conf/callsign.cfg
        rm ${D}/${prj}/conf/addr.cfg
        cd -
       
        cd ${D}/${prj}/scripts
        chmod +x *.sh
        cd -
        
        cd ${WORKDIR}	
	#install the service file to /etc/systemd/system
	install -Dm 644 ${prj}.service ${D}${service_dir}/${prj}.service

	#install the logrotate configuration file
	install -Dm 644 logrotate.d/${prj} ${D}/etc/logrotate.d/${prj}
	cd -
	
}

