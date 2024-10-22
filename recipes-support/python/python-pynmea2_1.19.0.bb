
SUMMARY = "Python library for the NMEA 0183 protcol"
HOMEPAGE = "https://github.com/Knio/pynmea2"
AUTHOR = "Tom Flanagan <tom@zkpq.ca>"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=bb5e173bc54080cb25079199959ba6b6"

SRC_URI = "https://files.pythonhosted.org/packages/c0/d7/5d0e36d00cd8aa19a6e717bcd1216ca805d86865501dfebdd18667c9edbd/pynmea2-1.19.0.tar.gz"
SRC_URI[md5sum] = "c212570e79bac99e6c9fb7b89e8c04ed"
SRC_URI[sha256sum] = "1daa79b93279f887d1c235e5cc5c79e32644564138ce46989ab0f4a2fc970d7c"

S = "${WORKDIR}/pynmea2-1.19.0"

RDEPENDS_${PN} = ""

inherit setuptools3
