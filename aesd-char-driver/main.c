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
#include <linux/slab.h>    // for kmalloc and kfree
#include <linux/uaccess.h> // for copy_to_user and copy_from_user
#include <linux/mutex.h>


#include "aesdchar.h"


int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Shruthi Thallapally"); /** TODO: fill in your name **/
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int aesd_open(struct inode *inode, struct file *filp)
{
    
   
    
    PDEBUG("open");
    filp->private_data = container_of(inode->i_cdev, struct aesd_dev, cdev); 
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    PDEBUG("release");
    
      filp->private_data =NULL;
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
     struct aesd_dev *dev ;
    struct aesd_buffer_entry *temp;
    size_t entry_offset = 0;
    size_t bytes_to_read;
    if (filp == NULL || buf == NULL || f_pos == NULL || *f_pos < 0)
    {
        return -EINVAL;
    }
    
     dev= filp->private_data;
     PDEBUG("read %zu bytes with offset %lld",count,*f_pos);
    
    if(mutex_lock_interruptible(&dev->lock)!=0)
    {
    return -ERESTART;
   
    }
   
   
     temp = aesd_circular_buffer_find_entry_offset_for_fpos(&dev->buffer, *f_pos, &entry_offset);
    if (!temp) {
        retval = 0; // No more data to read
        goto unlock_out;
    }
    
    bytes_to_read = min(count, temp->size - entry_offset);
    if (copy_to_user(buf, temp->buffptr + entry_offset, bytes_to_read)) {
        retval = -EFAULT;
        goto unlock_out;
    }
    
    *f_pos += bytes_to_read; // Update file position
    retval = bytes_to_read;

unlock_out:
    mutex_unlock(&dev->lock);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_dev *dev ; 
  //  struct aesd_buffer_entry *entry;
    char *write_buffer=NULL;
    const char *nwline_ptr=NULL;
    char *temp_ptr=NULL;
    const char *ret_ptr;
   size_t buff_size=0;
   PDEBUG("write %zu bytes with offset %lld",count,*f_pos); 
    if (filp == NULL || buf == NULL || f_pos == NULL || count <= 0 || *f_pos<0)
    {
        return -EINVAL;
       
    }
  
    dev= filp->private_data;
    if(dev == NULL)
    {
    	return -EINVAL;
    	
    }
   
    write_buffer = kmalloc(count, GFP_KERNEL); // Allocate memory for incoming data
    if (write_buffer==NULL)
    {
      return -ENOMEM;
    
	}
    if (copy_from_user(write_buffer, buf, count)) { // Copy data from user space
        
        retval= -EFAULT;
        goto free_out;
    }
    nwline_ptr=memchr(write_buffer,'\n',count);
    buff_size = nwline_ptr ? nwline_ptr - write_buffer + 1 : 0;

     if(mutex_lock_interruptible(&dev->lock)!=0)
    {
    	retval=-ERESTART;
    	goto free_out;
    }
    if(buff_size>0)
    {
    
     dev->entry.buffptr=krealloc(dev->entry.buffptr, dev->entry.size + buff_size, GFP_KERNEL);
     
     if (dev->entry.buffptr == NULL) 
     {
            PDEBUG("Error: Reallocation failed\n");
            retval = -ENOMEM;
            goto free_unlock_out;
           }
 	 memcpy((void *)(dev->entry.buffptr + dev->entry.size), write_buffer, buff_size);
        dev->entry.size += buff_size;
        ret_ptr = aesd_circular_buffer_add_entry(&dev->buffer, &dev->entry);
        if (ret_ptr) {
            kfree(ret_ptr);
        }
        dev->entry.size = 0;
        dev->entry.buffptr = NULL;
            
    }
    else
    {
     dev->entry.buffptr = krealloc(dev->entry.buffptr, dev->entry.size + count, GFP_KERNEL);
        if (dev->entry.buffptr == NULL) {
            PDEBUG("Error: Reallocation failed\n");
            retval = -ENOMEM;
    	goto free_unlock_out;
    }
    
    memcpy((void *)(dev->entry.buffptr+dev->entry.size),write_buffer,count);
    dev->entry.size+=count;
    
    }
    retval=count;
free_unlock_out:
    mutex_unlock(&dev->lock);
free_out:
	kfree(write_buffer);

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

       aesd_circular_buffer_init(&aesd_device.buffer);
     mutex_init(&aesd_device.lock);


    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
uint8_t index=0;
    struct aesd_buffer_entry *entry;
    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);
    
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.buffer, index)
    {
    	if(entry->buffptr != NULL)
    	{
        	kfree(entry->buffptr);
    	}
    }
    
    mutex_destroy(&aesd_device.lock);

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
