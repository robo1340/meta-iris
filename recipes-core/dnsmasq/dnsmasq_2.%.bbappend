FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SYSTEMD_AUTO_ENABLE:${PN} = "disable"

SRC_URI += "file://dnsmasq.conf"
SRC_URI += "file://override.conf"

conf_path_no_d = "/etc"
conf_path = "${D}${conf_path_no_d}"
override_path = "/etc/systemd/system/dnsmasq.service.d"

FILES:${PN} += "${conf_path_no_d}/*"

do_install:append() {
	install -d ${conf_path}
	install -d ${D}${override_path}

	install -m 0644 ${WORKDIR}/dnsmasq.conf ${conf_path}/dnsmasq.conf
	install -m 0644 ${WORKDIR}/override.conf ${D}${override_path}/override.conf
}
