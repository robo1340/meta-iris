DESCRIPTION = "Recipe to build and install the tiles for the  nodejs web service "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

#RM_WORK_EXCLUDE += "${PN}"

# Create dependency to library packages
RDEPENDS:${PN} += " "
PACKAGES="${PN}"

#src uri's for the source code and .service conf file
SRC_URI +="file://LICENSE"
SRC_URI +="file://tiles"

S = "${WORKDIR}"

#skip this part of the qa check
do_package_qa[noexec] = "1"


#add dir to the package files variable
FILES:${PN} += "/home/tiles/*"

do_install(){ 
	cd ${WORKDIR} 
        find tiles -type f -exec install -Dm 644 "{}" "${D}/home/{}" \;	        
        cd - 
}

