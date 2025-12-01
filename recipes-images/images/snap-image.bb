DESCRIPTION = "Image for Iris with snap radio layer"
LICENSE = "MIT"

#inherit extrausers systemd

# =========================================================================
# Copied from st-image-core
# =========================================================================

SUMMARY = "OpenSTLinux core image."
LICENSE = "Proprietary"

include recipes-st/images/st-image.inc

inherit core-image

IMAGE_LINGUAS = "en-us"

IMAGE_FEATURES += "package-management"
#ssh-server-dropbear

#
# INSTALL addons
#
CORE_IMAGE_EXTRA_INSTALL += " \
    resize-helper \
    \
    packagegroup-framework-core-base    \
    packagegroup-framework-tools-base   \
    \
    ${@bb.utils.contains('COMBINED_FEATURES', 'optee', 'packagegroup-optee-core', '', d)}   \
    ${@bb.utils.contains('COMBINED_FEATURES', 'optee', 'packagegroup-optee-test', '', d)}   \
    "

# =========================================================================
# Configure STM32MP default version to github
# =========================================================================
#STM32MP_SOURCE_SELECTION:pn-linux-stm32mp = "github"
#STM32MP_SOURCE_SELECTION:pn-optee-os-stm32mp = "github"
#STM32MP_SOURCE_SELECTION:pn-tf-a-stm32mp = "github"
#STM32MP_SOURCE_SELECTION:pn-u-boot-stm32mp = "github"

CORE_IMAGE_EXTRA_INSTALL:append = "openssh openssh-sftp openssh-sftp-server"

IMAGE_INSTALL:append = " packagegroup-core-buildessential"
IMAGE_INSTALL:append = " vim "
IMAGE_INSTALL:append = " gdb "

IMAGE_INSTALL:append = " zeromq-dev zeromq libczmq-dev czmq "
IMAGE_INSTALL:append = " python3-pyserial python3-pyzmq python3-pip tcpdump " 
IMAGE_INSTALL:append = " dtc "
IMAGE_INSTALL:append = " net-tools socat "
IMAGE_INSTALL:append = " python3-spidev python3-pyserial python3-sliplib "
IMAGE_INSTALL:append = " linux-firmware-ath9k "
IMAGE_INSTALL:append = " nodejs frr libmicrohttpd random-mac wifi gps " 

IMAGE_INSTALL:append = " snap hub node " 
#IMAGE_INSTALL:append = " bridge " 
#IMAGE_INSTALL:append = " chat " 
IMAGE_INSTALL:append = " bash " 
#IMAGE_INSTALL:append = " bridge-src " 
#IMAGE_INSTALL:append = " apache2 " 
#IMAGE_INSTALL:append = " nginx " 

