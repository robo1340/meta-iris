DESCRIPTION = "install a oneshot service that resets the wifi usb dongle after boot"
inherit systemd
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="MIT"

#source code and default configuration files
SRC_URI +="file://wifi_reset.service"
SRC_URI +="file://wifi_reset.sh"
SRC_URI +="file://LICENSE"
LIC_FILES_CHKSUM = "file://${WORKDIR}/LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_SERVICE:${PN} = "wifi_reset.service"

#skip this part of the qa check
#do_package_qa[noexec] = "1"

#base installation directory of the recipe
service_dir = "/etc/systemd/system"
name = "wifi_reset"

#add cellular_dir to the package files variable
FILES:${PN} += "${service_dir}/${name}.service "
FILES:${PN} += "/root/${name}.sh"

PACKAGES = "${PN}"

do_install(){ 
  #navigate to the working directory and install
	cd ${WORKDIR}
	#install the service file to /etc/systemd/system
	install -Dm 644 ${name}.service ${D}${service_dir}/${name}.service
	install -Dm 755 ${name}.sh ${D}/root/${name}.sh
	cd -
}

