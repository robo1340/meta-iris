do_install:append(){
	cd ${D}/etc 
	ln -s ./fw_env.config.mmc ./fw_env.config
	cd -
}
