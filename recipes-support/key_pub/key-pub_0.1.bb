DESCRIPTION = "Recipe to build and install the service that send key events over the radio "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"
RM_WORK_EXCLUDE += "${PN}"
# project name
prj="key_pub"

# Create dependency to library packages
RDEPENDS:${PN} += " python3-pyzmq python-sshkeyboard "
RDEPENDS:${PN} += " hub  "
DEPENDS = " "

#src uri's for the source code and .service conf file
SRC_URI +="file://${prj}"
SRC_URI +="file://LICENSE"

S = "${WORKDIR}"
PACKAGES = "${PN}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

INSANE_SKIP:${PN}:append = "already-stripped"

#add dir to the package files variable
#FILES:${PN} += "${service_dir}/* "
FILES:${PN} += "/${prj}"

do_install(){
 
  #navigate to the working directory where everything was compiled and install
  #all the files and subdirectories in the proxy directory  
	cd ${WORKDIR}
	find ${prj} -type f -exec install -Dm 644 "{}" "${D}/{}" \;
       
        cd -
       
        cd ${D}/${prj}/scripts
        chmod +x *.sh
        cd -
        	
}

