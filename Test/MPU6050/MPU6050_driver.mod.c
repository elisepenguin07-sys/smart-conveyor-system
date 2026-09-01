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
	{ 0x92997ed8, "_printk" },
	{ 0x0c464cde, "i2c_unregister_device" },
	{ 0xb4b4657d, "i2c_put_adapter" },
	{ 0x28bb7422, "device_destroy" },
	{ 0xf3a652a3, "class_unregister" },
	{ 0x09474a06, "class_destroy" },
	{ 0x6bc3fbc0, "__unregister_chrdev" },
	{ 0xdcb764ad, "memset" },
	{ 0x7c331728, "__register_chrdev" },
	{ 0xc1c92698, "class_create" },
	{ 0x9d630eab, "device_create" },
	{ 0xfb0f2841, "i2c_get_adapter" },
	{ 0x780a896c, "i2c_new_client_device" },
	{ 0x1379a871, "i2c_transfer_buffer_flags" },
	{ 0xf0fdf6cb, "__stack_chk_fail" },
	{ 0x6cbbfc54, "__arch_copy_to_user" },
	{ 0x91d66ee9, "module_layout" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "88CF2C5C82A299FFC9AA6CB");
