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
	{ 0xdee352ff, "__platform_driver_register" },
	{ 0x28bb7422, "device_destroy" },
	{ 0x09474a06, "class_destroy" },
	{ 0x6d86443f, "cdev_del" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0x54164f9e, "pwm_apply_might_sleep" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x47c11e94, "cdev_init" },
	{ 0xee983164, "cdev_add" },
	{ 0xc1c92698, "class_create" },
	{ 0x9d630eab, "device_create" },
	{ 0x595ea9a4, "devm_pwm_get" },
	{ 0x8f7ad95f, "_dev_info" },
	{ 0x3df967b6, "_dev_err" },
	{ 0xdbd4b560, "platform_driver_unregister" },
	{ 0xdcb764ad, "memset" },
	{ 0x12a4e128, "__arch_copy_from_user" },
	{ 0x5c3c7387, "kstrtoull" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("of:N*T*Craspberrypi,rpi5-pwm-custom");
MODULE_ALIAS("of:N*T*Craspberrypi,rpi5-pwm-customC*");

MODULE_INFO(srcversion, "BB5C272A0F7F4B8D464112F");
