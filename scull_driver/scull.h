#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/module.h> 	
#include <linux/init.h> 	
#include <linux/fs.h> 		
#include <linux/cdev.h> 	
#include <linux/slab.h> 	
#include <asm/uaccess.h>

extern int scull_major;		
extern int scull_minor;		
extern int scull_nr_devs;		
extern int scull_quantum;	
extern int scull_qset;
extern int max_size;

extern struct scull_dev *scull_device;
extern struct file_operations scull_fops;

extern struct wait_queue_head read_queue;
extern struct wait_queue_head write_queue;

struct scull_qset {
	void **data;			
	struct scull_qset *next; 	
};

struct scull_dev {
	struct scull_qset *data;  
	int quantum;		 
	int qset;		  
	unsigned long size;	  
	unsigned int access_key;  
	struct semaphore sem;    
	struct cdev cdev;
	unsigned int readp;
};

int scull_trim(struct scull_dev *dev);
int scull_open(struct inode *inode, struct file *filp);
int scull_release(struct inode *inode, struct file *filp);
struct scull_qset *scull_follow(struct scull_dev *dev, int n);
ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
loff_t scull_llseek(struct file *filp, loff_t off, int whence);

#endif /* _SCULL_H_ */