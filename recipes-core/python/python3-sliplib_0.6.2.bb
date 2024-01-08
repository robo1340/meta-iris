
SUMMARY = "Slip package"
HOMEPAGE = "https://github.com/rhjdjong/SlipLib"
AUTHOR = "Ruud de Jong <ruud.de.jong@xs4all.nl>"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://setup.py;md5=ca4c29086680f35dda5ac9a2736740be"

SRC_URI = "https://files.pythonhosted.org/packages/a5/69/23ac21075ef1757a9bd10c9f01df0a06c41ec34384db2c0b7cbbfd6b3ede/sliplib-0.6.2.tar.gz"
SRC_URI[md5sum] = "c6f22d088305f757b853b75b9218ff3b"
SRC_URI[sha256sum] = "443f480339927c2a192783626dcab6eb1726e2ec78ce35405ff4935316138b29"

S = "${WORKDIR}/sliplib-0.6.2"

RDEPENDS_${PN} = ""

inherit setuptools3
