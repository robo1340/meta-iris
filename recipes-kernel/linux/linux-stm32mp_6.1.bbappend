FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI += "file://0001-added-iris.dtb-to-Makefile.patch "
SRC_URI += "file://iris.dts "

#SRC_URI += "file://iris_config;subdir=fragments "
#SRC_URI += "file://iris.config "
SRC_URI:append = " file://iris.config;subdir=fragments "

#KERNEL_CONFIG_FRAGMENTS:append = "${WORKDIR}/fragments/${LINUX_VERSION}/iris.config"
KERNEL_CONFIG_FRAGMENTS:append = "${WORKDIR}/fragments/iris.config"

#do_configure:prepend(){
#    echo "overwriting kernel config"
#    cp ${WORKDIR}/iris_config ${WORKDIR}/defconfig
#    cp ${WORKDIR}/defconfig ${WORKDIR}/build/.config 
#}

do_configure:append(){
 cp ${WORKDIR}/iris.dts ${S}/arch/arm/boot/dts
}
