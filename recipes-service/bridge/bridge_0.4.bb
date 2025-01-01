DESCRIPTION = "Recipe to build and install the slip ip radio bridge "
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
LICENSE="LICENSE"
LIC_FILES_CHKSUM = "file://LICENSE;md5=2ba5c6e4c336f3d9e4d191d4a74fb91c"
RM_WORK_EXCLUDE += "${PN}"
# project name
prj="bridge"

inherit systemd

# Create dependency to library packages
RDEPENDS:${PN} += " zeromq zeromq-dev czmq libczmq-dev "
RDEPENDS:${PN} += " libgpiod  libgpiod-dev "
RDEPENDS:${PN} += " libserialport libserialport-dev "
RDEPENDS:${PN} += " libturbofec libturbofec-dev "
RDEPENDS:${PN} += " net-tools socat hostapd dnsmasq wifi-reset logrotate "
RDEPENDS:${PN} += " iproute2 iw busybox  "
DEPENDS = " zeromq czmq libgpiod libturbofec libserialport net-tools socat "

#src uri's for the source code and .service conf file
SRC_URI +="file://bridge"
SRC_URI +="file://${prj}.service"
SRC_URI +="file://logrotate.d"
SRC_URI +="file://LICENSE"

SYSTEMD_AUTO_ENABLE:${PN} = "enable"
SYSTEMD_SERVICE:${PN} = "${prj}.service"

S = "${WORKDIR}"
PACKAGES = "${PN}"

#skip this part of the qa check
do_package_qa[noexec] = "1"

#do_compile(){
#	#cross-compile proxy binary	
#	#cd ${WORKDIR}/${prj}/src/prj-${prj}
#	#make clean
#	#make	
#	#cd -	
#	#cd ${WORKDIR}/${prj}/src/prj-zmq-send
#	#make clean
#	#make	
#	#cd -	
#	#cd ${WORKDIR}/${prj}/src/prj-zmq-recv
#	#make clean
#	#make	
#	#cd -
#}

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
       
	install -d ${D}/bridge/conf/si4463/other
	install -d ${D}/bridge/conf/si4463/modem
        cd -
      
	rm ${D}/bridge/conf/radio_*.json
 
        cd ${D}/bridge/conf/si4463/modem 
        rm 1000_modem_config.h 
        ln -s ../all/wds_generated/radio_config_Si4463_430M_10kbps.h  1000_modem_config.h      
        cd -
        cd ${D}/bridge/conf/si4463/other
        rm 9000_general.h
        rm 9000_packet.h
        rm 9000_preamble.h	
        ln -s ../all/general/default.h 9000_general.h
 	ln -s ../all/packet/fsk.h 9000_packet.h
 	ln -s ../all/preamble/default.h 9000_preamble.h
	cd -

	cd ${D}/bridge/src/radio/inc
        rm si446x_patch.h	
        ln -s si446x_patch_revC.h si446x_patch.h
	cd -

        cd ${D}/bridge/scripts
        chmod +x *.sh
        cd -
	
	#install the service file to /etc/systemd/system
	install -Dm 644 ${prj}.service ${D}${service_dir}/${prj}.service

	#install the logrotate configuration file
	install -Dm 644 logrotate.d/${prj} ${D}/etc/logrotate.d/${prj}

	#install the compiled binary
        install -Dm 755 ${prj}/src/prj-${prj}/test ${D}/${prj}/src/prj-${prj}/test	
        install -Dm 755 ${prj}/src/prj-zmq-send/test ${D}/${prj}/src/prj-zmq-send/test	
        install -Dm 755 ${prj}/src/prj-zmq-recv/test ${D}/${prj}/src/prj-zmq-recv/test	
        
	install -Dm 755 ${prj}/scripts/virtual_tty_setup.sh ${D}/${prj}/scripts/virtual_tty_setup.sh	
	install -Dm 755 ${prj}/scripts/kill_virtual_tty.sh ${D}/${prj}/scripts/kill_virtual_tty.sh	
	install -Dm 755 ${prj}/scripts/sync_radio_conf.sh ${D}/${prj}/scripts/sync_radio_conf.sh	
	install -Dm 755 ${prj}/scripts/sync_scripts.sh ${D}/${prj}/scripts/sync_scripts.sh	
	install -Dm 755 ${prj}/scripts/sync_src.sh ${D}/${prj}/scripts/sync_src.sh	
	cd -
	
}

