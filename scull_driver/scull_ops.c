#include "scull.h"

struct file_operations scull_fops = {		
	.owner = THIS_MODULE,			
	.read = scull_read,
	.write = scull_write,
	.open = scull_open,
	.release = scull_release,
	.llseek = scull_llseek
};

int scull_trim(struct scull_dev *dev) {
	struct scull_qset *next, *dptr;
	int qset = dev->qset; 
	int i;

	for (dptr = dev->data; dptr; dptr = next) { 
		if (dptr->data) {
			for (i = 0; i < qset; i++)
				kfree(dptr->data[i]);

			kfree(dptr->data);
			dptr->data = NULL;
		}

		next = dptr->next;
		kfree(dptr);
	}

	dev->size = 0;
	dev->quantum = scull_quantum;
	dev->qset = scull_qset;
	dev->data = NULL;

	return 0;
}

int scull_open(struct inode *inode, struct file *flip) {
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	flip->private_data = dev;

	if (dev->user_count >= max_user_count) {
		printk(KERN_ALERT "scull: device is busy (maximum number of users - 4)\n");
		return -EINTR;
	}

	if ((flip->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;

		scull_trim(dev);
		up(&dev->sem);
	}
	
	printk(KERN_INFO "scull: device is opened\n");
	
	dev->user_count++;

	return 0;
}

int scull_release(struct inode *inode, struct file *flip) {
	struct scull_dev *dev = container_of(inode->i_cdev, struct scull_dev, cdev);

	printk(KERN_INFO "scull: device is released\n");
	
	dev->user_count--;

	return 0;
}

ssize_t scull_read(struct file *flip, char __user *buf, size_t count, loff_t *f_pos) {
	struct scull_dev *dev = flip->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t rv = 0;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if (*f_pos >= dev->size) {	
		printk(KERN_INFO "scull: *f_pos more than size %lu\n", dev->size);
		goto out;
	}

	if (*f_pos + count > dev->size) {
		printk(KERN_INFO "scull: correct count\n");	
		count = dev->size - *f_pos;
	}

	item = (long)*f_pos / itemsize;	
	rest = (long)*f_pos % itemsize;	

	s_pos = rest / quantum;		
	q_pos = rest % quantum;		

	dptr = scull_follow(dev, item);	

	if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
		goto out;

	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
		rv = -EFAULT;
		goto out;
	}

	*f_pos += count;
	
	rv = count;

out:
	up(&dev->sem);
	return rv;
}

ssize_t scull_write(struct file *flip, const char __user *buf, size_t count, loff_t *f_pos) {
	struct scull_dev *dev = flip->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t rv = -ENOMEM;


	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	item = (long)*f_pos / itemsize;
	rest = (long)*f_pos % itemsize;
	
	s_pos = rest / quantum;
	q_pos = rest % quantum;

	dptr = scull_follow(dev, item);

	if (dptr == NULL)
		goto out;

	if (!dptr->data) {
		dptr->data = kmalloc(qset * sizeof(char *), GFP_KERNEL);

		if (!dptr->data)
			goto out;
		
		memset(dptr->data, 0, qset * sizeof(char *));	
	}	

	if (!dptr->data[s_pos]) {
		dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
		
		if (!dptr->data[s_pos])
			goto out;
	}


	if (count > quantum - q_pos)
		count = quantum - q_pos;

	if (copy_from_user(dptr->data[s_pos] + q_pos, buf, count)) {
		rv = -EFAULT;
		goto out;
	}

	*f_pos += count;
	rv = count;

	if (dev->size < *f_pos)
		dev->size = *f_pos;
	
out:
	up(&dev->sem);
	return rv;
}

loff_t scull_llseek(struct file *flip, loff_t off, int whence) {
	struct scull_dev *dev = flip->private_data;
	loff_t new_off;

	switch(whence) {
	case 0: /* SEEK_SET */
		new_off = off;
		break;

	case 1: /* SEEK_CUR */
		new_off = flip->f_pos + off;
		break;

	case 2: /* SEEK_END */
		new_off = dev->size + off;
		break;

	default: 
		return -EINVAL;
	}
	if (new_off < 0) return -EINVAL;

	flip->f_pos = new_off;

	return new_off;
}

struct scull_qset *scull_follow(struct scull_dev *dev, int n) {
	struct scull_qset *qs = dev->data;

	if (!qs) {
		qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
		
		if (qs == NULL)
			return NULL;
		
		memset(qs, 0, sizeof(struct scull_qset));
	}

	while (n--) {
		if (!qs->next) {
			qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
			
			if (qs->next == NULL)
				return NULL;

			memset(qs->next, 0, sizeof(struct scull_qset));	
		}
		
		qs = qs->next;
		continue;	
	}
	
	return qs;
}