deps_config := \
	lib/Kconfig \
	boot/Kconfig \
	Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
