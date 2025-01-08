FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

#disable networkd
#PACKAGECONFIG_remove = "networkd"
#PACKAGECONFIG[networkd] = "-Dnetworkd=false"

SRC_URI += " file://frr.conf "

FILES:${PN} += " /etc/frr/frr.conf "

do_install:append() {
    install -d ${D}/etc/frr
    install -m 0644 ${WORKDIR}/frr.conf ${D}/etc/frr/frr.conf

    #install -d ${D}${sysconfdir}/systemd/network
    #install -m 0644 ${WORKDIR}/20-wired.network ${D}${sysconfdir}/systemd/network/

    #set the vendor preset so that sshd.socket is disabled by default
    #rm -rf ${D}/etc/systemd/system/sockets.target.wants/sshd.socket

}

SYSTEMD_SERVICE_${PN} = "enable"
