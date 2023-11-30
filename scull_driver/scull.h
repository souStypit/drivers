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
extern unsigned short max_user_count;

extern struct scull_dev *scull_device;
extern struct file_operations scull_fops;

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
	unsigned short user_count;
};

int scull_trim(struct scull_dev *dev);
int scull_open(struct inode *inode, struct file *flip);
int scull_release(struct inode *inode, struct file *flip);
struct scull_qset *scull_follow(struct scull_dev *dev, int n);
ssize_t scull_read(struct file *flip, char __user *buf, size_t count, loff_t *f_pos);
ssize_t scull_write(struct file *flip, const char __user *buf, size_t count, loff_t *f_pos);
loff_t scull_llseek(struct file *flip, loff_t off, int whence);

#endif /* _SCULL_H_ */