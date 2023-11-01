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

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Amey More"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    struct aesd_dev *dev = NULL;
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    dev = container_of(inode->i_cdev, struct aesd_dev, cdev);
    filp->private_data = dev; 
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    /**
     * TODO: handle release
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
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    
    // check arguments
    if ((filp == NULL) || (buf == NULL))
    {
        PDEBUG("aesd_read() invalid arguments");
        return -EINVAL;
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
            return -EINVAL;
        }
        else
        {
            retval = (bytes_to_copy - retval);
            *f_pos += retval;
        }
    }
    
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev = NULL;
    const char *free_buffptr = NULL;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
     
    // check arguments
    if ((filp == NULL) || (buf == NULL))
    {
        PDEBUG("aesd_read() invalid arguments");
        return -EINVAL;
    }
    
    dev = filp->private_data;
    
    // kernel malloc and error check
    dev->entry.buffptr = kmalloc(count, GFP_KERNEL);
    if(dev->entry.buffptr == NULL)
    {
	PDEBUG("kmalloc() error");
	return -ENOMEM;
    }
    
    // acquire lock
    if (mutex_lock_interruptible(&dev->lock) != 0)
    {
        PDEBUG("mutex_lock_interruptible() acquiring lock error");
        return -ERESTARTSYS;
    }
    
    // run commands once
    do
    {
    	retval = copy_from_user((void *)(dev->entry.buffptr + dev->entry.size), buf, count);
    	if (retval != 0)
	{
	    PDEBUG("copy_from_user() error retval=%zu", retval);
	    break;
	}
    	retval = (count - retval);
    	dev->entry.size += retval;
    
    	if (dev->entry.buffptr[dev->entry.size-1] == '\n')
    	{
            free_buffptr = aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
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
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
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
     * TODO: initialize the AESD specific portion of the device
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
     * TODO: cleanup AESD specific poritions here as necessary
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
