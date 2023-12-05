#include "scull.h"

MODULE_AUTHOR("Savenkov");
MODULE_LICENSE("Dual BSD/GPL");

void scull_cleanup_module(void);
static void scull_setup_cdev(struct scull_dev *dev, int index);
static int scull_init_module(void);

int scull_major = 0;		
int scull_minor = 0;		
int scull_nr_devs = 2;		
int scull_quantum = 64;	
int scull_qset = 2;
unsigned short max_user_count = 4;
struct scull_dev *scull_device;

static void scull_setup_cdev(struct scull_dev *dev, int index) {
	int err, devno = MKDEV(scull_major, scull_minor + index);	

	cdev_init(&dev->cdev, &scull_fops);

	dev->cdev.owner = THIS_MODULE;
	dev->cdev.ops = &scull_fops;

	err = cdev_add(&dev->cdev, devno, 1);

	if (err)
		printk(KERN_NOTICE "Error %d adding scull  %d", err, index);
}

static int scull_init_module(void) {
	int rv, i;
	dev_t dev;
		
	rv = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");	

	if (rv) {
		printk(KERN_WARNING "scull: can't get major %d\n", scull_major);
		return rv;
	}

    scull_major = MAJOR(dev);

	scull_device = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);	
	
	if (!scull_device) {
		rv = -ENOMEM;
		goto fail;
	}

	memset(scull_device, 0, scull_nr_devs * sizeof(struct scull_dev));		

	for (i = 0; i < scull_nr_devs; i++) {						
		scull_device[i].quantum = scull_quantum;
		scull_device[i].qset = scull_qset;
		scull_device[i].user_count = 0;
		sema_init(&scull_device[i].sem, 1);
		scull_setup_cdev(&scull_device[i], i);					
	}

	dev = MKDEV(scull_major, scull_minor + scull_nr_devs);	
	
	printk(KERN_INFO "scull: major = %d minor = %d\n", scull_major, scull_minor);

	return 0;

fail:
	scull_cleanup_module();
	return rv;
}

void scull_cleanup_module(void) {
	int i;
	dev_t devno = MKDEV(scull_major, scull_minor);

	if (scull_device) {
		for (i = 0; i < scull_nr_devs; i++) {
			scull_trim(scull_device + i);		
			cdev_del(&scull_device[i].cdev);	
		}
		
		kfree(scull_device);
	}

	unregister_chrdev_region(devno, scull_nr_devs); 
	printk(KERN_ALERT "scull: exit\n");
}

module_init(scull_init_module);		
module_exit(scull_cleanup_module);	
