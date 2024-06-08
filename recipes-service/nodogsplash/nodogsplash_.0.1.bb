DESCRIPTION = "Recipe to build and install the chat service "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
#LICENSE="GNU"
#LIC_FILES_CHKSUM = "file://COPYING;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

# project name
prj="nodogsplash"
ver="nodogsplash-5.0.2"

inherit systemd

# Create dependency to library packages
DEPENDS += "libmicrohttpd"
RDEPENDS:${PN} += " libmicrohttpd "

#CLEANBROKEN = "1"
#src uri's for the source code and .service conf file
SRC_URI = "https://github.com/nodogsplash/nodogsplash/archive/refs/tags/v5.0.2.tar.gz \
           file://animu.jpg \
           file://LICENSE \
           file://0001-modified-http_micrhttpd.patch \
           file://0002-modified-splash.html.patch \
           file://0003-modified-status.html.patch \
           file://0004-modified-conf.patch \
           "
SRC_URI[sha256sum] = "908d3674e93726fdcefb4c3b6705c745753435df9d46425781a57e3f6b417797"
#SRC_URI +="file://splash.html"
#SRC_URI +="file://splash.css"
#SRC_URI +="file://nodogsplash.conf"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

service_dir = "/etc/systemd/system"
etc_dir = "/etc/nodogsplash"

#add dir to the package files variable
FILES:${PN} += "${service_dir}/* "
FILES:${PN} += "${etc_dir}/* "
FILES:${PN} += "/usr/bin/nodogsplash"

inherit pkgconfig
#EXTRA_OEMAKE += "'CFLAGS=${CFLAGS} -I/usr/include/ -I{S} -I${STAGING_INCDIR}'"
EXTRA_OEMAKE += "'CFLAGS=${CFLAGS} -I${STAGING_INCDIR} '"

do_compile(){
 echo ${STAGING_INCDIR} 
cd ${S}/${ver}/ 
 #make 
 oe_runmake
 cd -
}


do_install(){ 
	cd ${WORKDIR}/${ver}
        ls ${WORKDIR}/${ver}	
        echo ${WORKDIR}/${ver}	
	#install the service file to /etc/systemd/system
	install -Dm 644 debian/${prj}.service ${D}${service_dir}/${prj}.service
	install -Dm 644 resources/splash.html ${D}${etc_dir}/splash.html
	install -Dm 644 resources/splash.css  ${D}${etc_dir}/splash.css
	install -Dm 644 ../animu.jpg   ${D}${etc_dir}/animu.jpg
	install -Dm 644 resources/nodogsplash.conf  ${D}${etc_dir}/nodogsplash.conf
	install -Dm 755 nodogsplash  ${D}/usr/bin/nodogsplash
	
	cd -	
}

