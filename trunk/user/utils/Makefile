utils_dir_y =
utils_dir_  =

utils_dir_y			+= cpu_usage
utils_dir_y			+= switch

ifdef CONFIG_RA_HW_NAT
utils_dir_y			+= hw_nat
endif

all romfs:
	for i in $(utils_dir_y) ; do make -C $$i $@ || exit $?; done

clean:
	for i in `ls -d *` ; do \
		[ ! -d $$i ] || \
		$(MAKE) -C $$i clean ; \
	done
