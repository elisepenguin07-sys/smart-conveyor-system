#include <linux/module.h>
#include <linux/export-internal.h>
#include <linux/compiler.h>

MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};



static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0xd866500a, "gpiod_get" },
	{ 0xdda912f6, "gpiod_put" },
	{ 0xbf61f840, "gpiod_set_value" },
	{ 0x09474a06, "class_destroy" },
	{ 0x92997ed8, "_printk" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x7ccf3c7a, "spi_sync" },
	{ 0x9d630eab, "device_create" },
	{ 0xc1c92698, "class_create" },
	{ 0x09a1de1d, "driver_unregister" },
	{ 0xdcb764ad, "memset" },
	{ 0x402d998e, "spi_setup" },
	{ 0xd1faaa04, "__spi_register_driver" },
	{ 0x7c331728, "__register_chrdev" },
	{ 0x28bb7422, "device_destroy" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0xf9a482f9, "msleep" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("spi:spi-dummy-driver");

MODULE_INFO(srcversion, "9B85B200DB634973CD1FB5A");
