PACKAGECONFIG:append = " networkd resolved"
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

#disable networkd
#PACKAGECONFIG_remove = "networkd"
#PACKAGECONFIG[networkd] = "-Dnetworkd=false"

SRC_URI += " file://20-wired.network "
#SRC_URI += " file://timesyncd.conf "

FILES:${PN} += " ${sysconfdir}/systemd/network/20-wired.network "
#FILES_${PN} += " ${sysconfdir}/systemd/timesyncd.conf "

do_install:append() {
    install -d ${D}${sysconfdir}/systemd/network
    install -m 0644 ${WORKDIR}/20-wired.network ${D}${sysconfdir}/systemd/network

    #install -d ${D}${sysconfdir}/systemd/network
    #install -m 0644 ${WORKDIR}/20-wired.network ${D}${sysconfdir}/systemd/network/

    #set the vendor preset so that sshd.socket is disabled by default
    #rm -rf ${D}/etc/systemd/system/sockets.target.wants/sshd.socket

}
#SYSTEMD_SERVICE_${PN} = ""
