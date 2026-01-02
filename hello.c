#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Me");
MODULE_DESCRIPTION("A simple Hello World module");

static int __init hello_init(void) {
	printk(KERN_INFO "hello, Rasverry Po 5! Kernel Module Loaded. \n");
	return 0;
}

static void __exit hello_exit(void){
	printk(KERN_INFO "Goodbue, RAspverry PI 5! Kernel Module Unloaded. \n");
}

module_init(hello_init);
module_exit(hello_exit);
