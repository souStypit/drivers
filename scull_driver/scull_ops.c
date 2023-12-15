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

int scull_open(struct inode *inode, struct file *filp) {
	struct scull_dev *dev;

	dev = container_of(inode->i_cdev, struct scull_dev, cdev);
	filp->private_data = dev;

	if ((filp->f_flags & O_ACCMODE) == O_WRONLY) {
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;

		scull_trim(dev);
		up(&dev->sem);
	}
	
	printk(KERN_INFO "scull: device is opened\n");
	
	return 0;
}

int scull_release(struct inode *inode, struct file *filp) {
	printk(KERN_INFO "scull: device is released\n");
	
	return 0;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos) {
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t rv = 0;

	*f_pos = dev->readp;
	dev->readp += count;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	if (dev->size > max_size) {
		printk(KERN_INFO "buffer cleaning\n");
		count = 0;
		*f_pos = 0;
		dev->size = 0;
		filp->f_pos = 0;
		scull_trim(dev);
		goto out;
	}

	while (dev->size <= 0) {
        printk("buffer is empty");
        if (filp->f_flags & O_NONBLOCK) {
            up(&dev->sem);
            return -EAGAIN;
        }
		up(&dev->sem);
        wait_event_interruptible(read_queue, dev->size > 0);
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
	}

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
	wake_up_interruptible(&write_queue);
	return rv;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos) {
	struct scull_dev *dev = filp->private_data;
	struct scull_qset *dptr;
	int quantum = dev->quantum, qset = dev->qset;
	int itemsize = quantum * qset;
	int item, s_pos, q_pos, rest;
	ssize_t rv = -ENOMEM;

	*f_pos = dev->size;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	while (dev->size >= max_size) {
        printk(KERN_INFO "buffer is filled");
        if (filp->f_flags & O_NONBLOCK) {
            up(&dev->sem);
			return -EAGAIN;
		}
		up(&dev->sem);
        wait_event_interruptible(write_queue, dev->size < max_size);
        if (down_interruptible(&dev->sem))
            return -ERESTARTSYS;
	}

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

	// if (*f_pos + count >= max_size) { 
	// 	printk(KERN_INFO "out of range\n");
	// 	count = max_size - count;
	// }

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
    wake_up_interruptible(&read_queue);
	return rv;
}

loff_t scull_llseek(struct file *filp, loff_t off, int whence) {
	struct scull_dev *dev = filp->private_data;
	loff_t new_off;

	switch(whence) {
	case 0: /* SEEK_SET */
		new_off = off;
		break;

	case 1: /* SEEK_CUR */
		new_off = filp->f_pos + off;
		break;

	case 2: /* SEEK_END */
		new_off = dev->size + off;
		break;

	default: 
		return -EINVAL;
	}
	if (new_off < 0) return -EINVAL;

	filp->f_pos = new_off;

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