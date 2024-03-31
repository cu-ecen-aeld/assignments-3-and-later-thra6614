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
 * heavily referenced from Suraj
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include "aesdchar.h"
#include "linux/slab.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Tommy Ramirez"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("open");
    /**
     * TODO: handle open
     */
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev);
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
    size_t  remainder;
    size_t  copy_num;
    struct aesd_buffer_entry *entry;
    size_t offset = 0;
    struct aesd_dev *cir_buff = filp->private_data;
    PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    /**
     * TODO: handle read
     */
    

    if(!filp || !buf || !f_pos) //invalid inputs
    {
        PDEBUG("Error: invalid inputs");
        return -EINTR;
    }
    

    if(mutex_lock_interruptible(&cir_buff->lock) != 0)
    {
        PDEBUG("Error: mutex could not lock properly");
        return -ERESTARTSYS; 
    }

    entry = aesd_circular_buffer_find_entry_offset_for_fpos(&cir_buff->buffer, *f_pos, &offset);

    if(entry)
    {
        remainder = entry->size - offset;
        copy_num = remainder;
        if(copy_num > count)
        {
            copy_num = count;
        }
        if (copy_to_user(buf, entry->buffptr + offset, copy_num))
        {
            PDEBUG("Error some bytes could not be copied");
            retval = -EFAULT;
        }
        else
        {
            retval = copy_num;
            *f_pos += retval;
        }
    }

    mutex_unlock(&cir_buff->lock);
    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *cir_buff = filp->private_data;
    bool newline_exists = false;
    char *write_data_buffer = NULL;
    size_t idx;
    size_t append_idx = 0;

    PDEBUG("write %zu bytes with offset %lld", count, *f_pos);
    /**
     * TODO: handle write
     */
    if(!filp || !buf || !f_pos) //invalid inputs
    {
        PDEBUG("Error: invalid inputs");
        return -EINTR;
    }
    if (count == 0)
    {
        return 0;
    }
    write_data_buffer = kmalloc(count, GFP_KERNEL); //dynamically allocate memory for storing write data
    if(!write_data_buffer)
    {
        PDEBUG("Error: malloc failed");
        retval = -ENOMEM;
        return retval;
    }

    if (copy_from_user(write_data_buffer, buf, count)) //copy data from kernal to user space
    {
        PDEBUG("Error some bytes could not be copied");
        retval = -EFAULT;
        kfree(write_data_buffer);
        mutex_unlock(&cir_buff->lock);
        return retval;
    }
    
    if(mutex_lock_interruptible(&cir_buff->lock) != 0) //lock mutex
    {
        PDEBUG("Error: mutex could not lock properly");
        return -ERESTARTSYS; 
    }
    
    
    for(idx = 0; idx < count; idx++) //search for first newline if present and store the index of the newline or the count value
    {
        if(write_data_buffer[idx] == '\n')
        {
            newline_exists = true;
            break;
        }
    }

    if(newline_exists == true) //determine kmalloc appending amount
    {
        append_idx = idx + 1;
        PDEBUG("Write data is %s", write_data_buffer);
    }
    else
    {
        append_idx = count;
        PDEBUG("Write data is %s", write_data_buffer);
    }
    
    
    
    //if write_buffer already has data stored, then realloc otherwise malloc
    if(cir_buff->write_buf_size == 0)
    {
        cir_buff->write_buf = kmalloc(count, GFP_KERNEL);
    }
    else
    {
        cir_buff->write_buf = krealloc(cir_buff->write_buf, (append_idx + cir_buff->write_buf_size), GFP_KERNEL);
    }
    if(cir_buff->write_buf == NULL) //confirm malloc/realloc worked
    {
        kfree(write_data_buffer);
        retval = -EFAULT;
        mutex_unlock(&cir_buff->lock);
        return retval;
    }    
    memcpy(cir_buff->write_buf + cir_buff->write_buf_size, write_data_buffer, append_idx); //append malloced data up to newline to write_buffer
    cir_buff->write_buf_size += append_idx; //update size with new data added

 
    //give a new buffer entry for the
    if(newline_exists)
    {
        struct aesd_buffer_entry entry;
        struct aesd_buffer_entry *to_be_freed = NULL;
        entry.size = cir_buff->write_buf_size;
        entry.buffptr = cir_buff->write_buf;

        if(cir_buff->buffer.full) //free 10th oldest write command
        {
            to_be_freed = &cir_buff->buffer.entry[cir_buff->buffer.out_offs]; //set to_be_freed as the entry that would've been freed first if read
            if(to_be_freed->buffptr) //remove from buffer
            {
                kfree(to_be_freed->buffptr);
            }
            to_be_freed->buffptr = NULL;
            to_be_freed->size = 0;
        }
        aesd_circular_buffer_add_entry(&cir_buff->buffer, &entry); //add to circular buffer
        cir_buff->buf_size += entry.size; //update circular buffer size
        cir_buff->write_buf_size = 0;
    }   
    
    retval = append_idx; //end of function successful write of multiple bytes
    
    //free the temp data malloc'ed at the beginning of the program
    if(write_data_buffer)
    {
        kfree(write_data_buffer);
    }

    mutex_unlock(&cir_buff->lock);
    return retval;
}
loof_t * llseek(struct file * filp, loff_t offset, int whence)
{
    struct mutex lock;
    mutex_init(&lock);
    if(mutex_lock_interruptible(&lock) != 0) //lock mutex
    {
        PDEBUG("Error: mutex could not lock properly");
        return -1; 
    }
    mutex_unlock(&lock);

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
    aesd_circular_buffer_init(&aesd_device.buffer);
    mutex_init(&aesd_device.lock); //init lock
    aesd_device.write_buf = NULL; //reset write buffer
    aesd_device.write_buf_size =0;
    result = aesd_setup_cdev(&aesd_device);
    


    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    dev_t devno = MKDEV(aesd_major, aesd_minor);
int8_t idx;
struct aesd_buffer_entry *entry;
    cdev_del(&aesd_device.cdev);

    /**
     * TODO: cleanup AESD specific poritions here as necessary
     */
    
    
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, idx){
        kfree(entry->buffptr);
    }
    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
