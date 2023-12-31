/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */
 
/**
 * @references
 * llseek -> https://github.com/cu-ecen-aeld/ldd3/blob/master/scull/main.c
 *
 */
 
#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Amey More");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = NULL;
    PDEBUG("open");
    /**
     * handle open
     */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; 
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * handle release
     */
    filp->private_data = NULL; 
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    ssize_t entry_offset = 0;
    struct aesd_buffer_entry *entry = NULL;
    ssize_t bytes_to_copy = 0;
    struct aesd_dev *dev = NULL;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * handle read
     */
    
    // check arguments
    if ((filp == NULL) || (buf == NULL))
    {
        PDEBUG("aesd_read() invalid arguments");
        return -EINVAL;
    }

    dev = filp->private_data;
	
    // acquire lock
    if (mutex_lock_interruptible(&dev->lock) != 0)
    {
        PDEBUG("mutex_lock_interruptible() acquiring lock error");
        return -ERESTARTSYS;
    }
    
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&aesd_device.buffer,*f_pos, &entry_offset);
    
    if(entry != NULL)
    {
    	bytes_to_copy = (entry->size - entry_offset);
	if (bytes_to_copy > count)
	{
	    bytes_to_copy = count;
	}
	
	retval = copy_to_user(buf, (entry->buffptr + entry_offset), bytes_to_copy);
	if (retval != 0)
	{
	    PDEBUG("copy_to_user() error retval=%zu", retval);
	}
	else
	{
	    PDEBUG("copy_to_user() success retval=%zu", retval);
	}
	retval = (bytes_to_copy - retval);
	*f_pos += retval;
    }
    
    // release lock
    mutex_unlock(&dev->lock);
    
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev *dev = NULL;
    const char *free_buffptr = NULL;
    
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * handle write
     */
     
    // check arguments
    if ((filp == NULL) || (buf == NULL))
    {
        PDEBUG("aesd_read() invalid arguments");
        return -EINVAL;
    }
    
    dev = filp->private_data;
    
    // acquire lock
    if (mutex_lock_interruptible(&dev->lock) != 0)
    {
        PDEBUG("mutex_lock_interruptible() acquiring lock error");
        return -ERESTARTSYS;
    }
    
    // run commands once
    do
    {
        // kernel malloc and error check
	dev->entry.buffptr = krealloc(dev->entry.buffptr, (dev->entry.size + count),
                                  GFP_KERNEL);//kmalloc(count, GFP_KERNEL);
	if(dev->entry.buffptr == NULL)
	{
		PDEBUG("krealloc() error");
		retval = -ENOMEM;
		break;
	}
    
    	retval = copy_from_user((void *)(dev->entry.buffptr + dev->entry.size), buf, count);
    	if (retval != 0)
	{
	    PDEBUG("copy_from_user() error retval=%zu", retval);
	    //break;
	}
    	retval = (count - retval);
    	dev->entry.size += retval;
    
    	if (dev->entry.buffptr[dev->entry.size-1] == '\n')
    	{
    		free_buffptr = aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
	        // free previously allocated entry if any
	        if (free_buffptr != NULL)
	        {
	            kfree(free_buffptr);
	            free_buffptr = NULL;
	        }
	        
	        dev->entry.buffptr = NULL;
	        dev->entry.size = 0;
	}
    }while(0);
    
    // release lock
    mutex_unlock(&dev->lock);
    
    return retval;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    int entry_index = 0;
    loff_t file_offset = 0;
    loff_t total_size = 0;
    struct aesd_dev *dev = NULL;
    struct aesd_buffer_entry *entry = NULL;
    
    PDEBUG("aesd_llseek()");

    dev = filp->private_data;
	
    // acquire lock
    if (0 != mutex_lock_interruptible(&dev->lock))
    {
        PDEBUG("ERROR: mutex_lock_interruptible acquiring lock");
        return -ERESTARTSYS;
    }

    // to get the total size
    AESD_CIRCULAR_BUFFER_FOREACH(entry,&aesd_device.buffer,entry_index)
    {
        total_size += entry->size;
    }
	
    // release lock
    mutex_unlock(&dev->lock);

    file_offset = fixed_size_llseek(filp, off, whence, total_size);
    
    return file_offset;
}

static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset)
{
	long retval = 0;
	int i;
	int entry_length;
	struct aesd_dev *dev = filp->private_data;
	struct aesd_buffer_entry *entry = NULL;
	
	PDEBUG("aesd_adjust_file_offset()");

	// acquire lock
    	if (mutex_lock_interruptible(&dev->lock) != 0)
	{
		PDEBUG("mutex_lock_interruptible() acquiring lock error");
		return -ERESTARTSYS;
	}
	
	do
	{
		PDEBUG("aesd_adjust_file_offset() start");
		// check if write_cmd exceeds max writes supported
		if (write_cmd > AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED)
		{		
			PDEBUG("invalid");
			retval = -EINVAL;
			break; // release lock and exit
	    	}
	    	
	    	// check if write_cmd exceeds no. of entries present
	    	AESD_CIRCULAR_BUFFER_FOREACH(entry,&aesd_device.buffer,entry_length);
		if (write_cmd > entry_length)
		{
			PDEBUG("invalid");
			retval = -EINVAL;
			break; // release lock and exit
	    	}
	    	
		// check if offset exceeds size of entry
		if (write_cmd_offset >= dev->buffer.entry[write_cmd].size)
		{
			PDEBUG("invalid");
			retval = -EINVAL;
			break; // release lock and exit
	    	}
	    	
	    	// adjust the file offset position
	    	for (i=0; i<write_cmd; i++)
		{
			filp->f_pos += dev->buffer.entry[i].size;
		}
		filp->f_pos += write_cmd_offset;
		
		PDEBUG("aesd_adjust_file_offset() completed");
		
	}while(0);

	// release lock
	mutex_unlock(&dev->lock);
	
	return retval;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long retval = 0;
 	struct aesd_seekto aesd_seekto_data;
 	
	PDEBUG("aesd_ioctl()");

	// check if the command passed is defined
	if (_IOC_TYPE(cmd) != AESD_IOC_MAGIC)
	{
		return -ENOTTY;
	}
	if (_IOC_NR(cmd) > AESDCHAR_IOC_MAXNR)
	{
		return -ENOTTY;
	}

 	switch (cmd)
 	{
		case AESDCHAR_IOCSEEKTO:
			retval = copy_from_user(&aesd_seekto_data, (const void __user *)arg, sizeof(aesd_seekto_data));
        		if (retval != 0)
        		{
            			retval = -EFAULT;
        		}
        		else
        		{
				retval = aesd_adjust_file_offset(filp, aesd_seekto_data.write_cmd, aesd_seekto_data.write_cmd_offset);
        		}
        	break;

 	    	default:
 			retval = -ENOTTY;
 			break;
 	}
 	
 	return retval;
}



struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek =   aesd_llseek,
    .unlocked_ioctl = aesd_ioctl,
};

static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev", err);
    }
    return err;
}



int aesd_init_module(void)
{
    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    /**
     * initialize the AESD specific portion of the device
     */
    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.buffer);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
    uint8_t i = 0;
    struct aesd_buffer_entry *entry = NULL;

    cdev_del(&aesd_device.cdev);

    /**
     * cleanup AESD specific poritions here as necessary
     */
    
    // free buffer
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, i)
    {
	if(entry->buffptr != NULL)
	{
		kfree(entry->buffptr);
		entry->buffptr = NULL;
	}
    }

    // Destroy the mutex
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
