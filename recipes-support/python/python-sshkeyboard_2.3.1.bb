
SUMMARY = "sshkeyboard"
HOMEPAGE = ""
AUTHOR = "Olli Paloviita <>"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=57fcb0cfcbbb87629c83ae71704a5ca6"

SRC_URI = "https://files.pythonhosted.org/packages/e3/7b/d78e6ade4bb4680d0a610ed02047c4de04db62de8864193bf842c59c47cb/sshkeyboard-2.3.1.tar.gz"
SRC_URI[md5sum] = "f8c08df60ce10776b03130e81a71370c"
SRC_URI[sha256sum] = "3273be5b2fde7f8d2ea075d40e1981104ac0928d7b77a848756f08d0e66a3d9f"

S = "${WORKDIR}/sshkeyboard-2.3.1"

RDEPENDS_${PN} = ""

inherit setuptools3
