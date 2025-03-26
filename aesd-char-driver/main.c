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
#include <linux/fs.h> // file_operations
#include <linux/slab.h>  // For kmalloc/kfree
#include <linux/string.h> // For memchr
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Bharath"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

loff_t aesd_llseek(struct file *filp, loff_t offset, int whence)
{
    struct aesd_dev *dev = filp->private_data;
    loff_t new_f_pos;
    size_t total_size = 0;
    size_t i;
    size_t valid_entries = (dev->circ_buffer.full) ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED : dev->circ_buffer.in_offs;
    
    mutex_lock(&dev->lock);
    /* Sum the sizes of all valid entries to compute total size */
    for (i = 0; i < valid_entries; i++) {
        size_t index = (dev->circ_buffer.out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        total_size += dev->circ_buffer.entry[index].size;
    }
    
    switch (whence) {
        case SEEK_SET:
            new_f_pos = offset;
            break;
        case SEEK_CUR:
            new_f_pos = filp->f_pos + offset;
            break;
        case SEEK_END:
            new_f_pos = total_size + offset;
            break;
        default:
            mutex_unlock(&dev->lock);
            return -EINVAL;
    }
    
    if (new_f_pos < 0) {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }
    
    filp->f_pos = new_f_pos;
    mutex_unlock(&dev->lock);
    return new_f_pos;
}

long aesd_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct aesd_dev *dev = filp->private_data;
    struct aesd_seekto seekto;
    size_t valid_entries;
    size_t offset = 0;
    size_t i;
    
    if (cmd != AESDCHAR_IOCSEEKTO)
        return -ENOTTY;
    
    if (copy_from_user(&seekto, (struct aesd_seekto *)arg, sizeof(seekto)))
        return -EFAULT;
    
    mutex_lock(&dev->lock);
    valid_entries = (dev->circ_buffer.full) ? AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED : dev->circ_buffer.in_offs;
    
    if (seekto.write_cmd >= valid_entries) {
        mutex_unlock(&dev->lock);
        return -EINVAL;
    }
    
    /* Calculate the absolute file offset by iterating over the commands
     * in the circular buffer (starting from out_offs).
     */
    for (i = 0; i < valid_entries; i++) {
        size_t index = (dev->circ_buffer.out_offs + i) % AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED;
        if (i == seekto.write_cmd) {
            if (seekto.write_cmd_offset >= dev->circ_buffer.entry[index].size) {
                mutex_unlock(&dev->lock);
                return -EINVAL;
            }
            offset += seekto.write_cmd_offset;
            break;
        } else {
            offset += dev->circ_buffer.entry[index].size;
        }
    }
    
    filp->f_pos = offset;
    mutex_unlock(&dev->lock);
    return 0;
}



int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    struct aesd_dev *dev;
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
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
     
    struct aesd_dev *dev = filp->private_data;
    size_t entry_offset;
    struct aesd_buffer_entry *entry;

    mutex_lock(&dev->lock);
    
    entry = aesd_circular_buffer_find_entry_offset_for_fpos(
        &dev->circ_buffer, *f_pos, &entry_offset);
    
    if(entry)
    {
        size_t bytes_to_read = min(count, entry->size - entry_offset);
        if(copy_to_user(buf, entry->buffptr + entry_offset, bytes_to_read))
        {
            retval = -EFAULT;
        } else
        {
            retval = bytes_to_read;
            *f_pos += bytes_to_read;
        }
    }
    
    mutex_unlock(&dev->lock);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    PDEBUG("write %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle write
     */
     
    struct aesd_dev *dev = filp->private_data;
    char *data = kmalloc(count, GFP_KERNEL);
    char *newline = NULL;
    
    if(!data) return retval;
    
    if(copy_from_user(data, buf, count))
    {
        retval = -EFAULT;
        goto out;
    }
    
    mutex_lock(&dev->lock);
    
    newline = memchr(data, '\n', count);
    
    if(newline)
    {
        size_t total_size = dev->partial_size + (newline - data + 1);
        char *combined = krealloc(dev->partial_write, total_size, GFP_KERNEL);
        
        if(!combined)
        {
            retval = -ENOMEM;
            goto unlock;
        }
        
        // Handle buffer full condition
        if(dev->circ_buffer.full) {
            kfree(dev->circ_buffer.entry[dev->circ_buffer.out_offs].buffptr);
        }
        
        memcpy(combined + dev->partial_size, data, newline - data + 1);
        struct aesd_buffer_entry entry = {
            .buffptr = combined,
            .size = total_size
        };
        aesd_circular_buffer_add_entry(&dev->circ_buffer, &entry);
        
        dev->partial_write = NULL;
        dev->partial_size = 0;
        retval = newline - data + 1;
    }
    else
    {
        dev->partial_write = krealloc(dev->partial_write, 
                                    dev->partial_size + count, GFP_KERNEL);
        if(!dev->partial_write)
        {
            retval = -ENOMEM;
            goto unlock;
        }
        memcpy(dev->partial_write + dev->partial_size, data, count);
        dev->partial_size += count;
        retval = count;
    }

unlock:
    mutex_unlock(&dev->lock);
out:
    kfree(data);

    return retval;
}
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
    .llseek          = aesd_llseek,
    .unlocked_ioctl  = aesd_ioctl,
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

    memset(&aesd_device, 0, sizeof(struct aesd_dev));
    mutex_init(&aesd_device.lock);
    aesd_circular_buffer_init(&aesd_device.circ_buffer);
    aesd_device.partial_write = NULL;
    aesd_device.partial_size = 0;

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */

    //Cleanup memory
    for(int i = 0; i < AESDCHAR_MAX_WRITE_OPERATIONS_SUPPORTED; i++) {
        kfree(aesd_device.circ_buffer.entry[i].buffptr);
    }
    if(aesd_device.partial_write)
    {
        kfree(aesd_device.partial_write);
    }
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
