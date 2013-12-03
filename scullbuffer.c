/* pipe.c -- fifo driver for scull*/
 
#include <linux/sched.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>	/* printk(), min() */
#include <linux/slab.h>		/* kmalloc() */
#include <linux/fs.h>		/* everything... */
#include <linux/proc_fs.h>
#include <linux/errno.h>	/* error codes */
#include <linux/types.h>	/* size_t */
#include <linux/fcntl.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include "scull.h"		/* local definitions */

#define init_MUTEX(_m) sema_init(_m, 1);

typedef struct it {
	char buffer[512];
	int num_bytes;
} item_t;

struct scull_bipe {
        wait_queue_head_t inq, outq;       /* read and write queues */
        item_t *buffer, *end;						//buffer
        //char *buffer, *end;                /* begin of buf, end of buf */
        int buffersize;                    /* used in pointer arithmetic */
        item_t *rp, *wp;                     /* where to read, where to write */
        int nreaders, nwriters;            /* number of openings for r/w */
        struct semaphore sem;              /* mutual exclusion semaphore */
        struct cdev cdev;                  /* Char device structure */
};

/* parameters */
static int scull_b_nr_devs = SCULL_B_NR_DEVS;	/* number of pipe devices */
int scull_b_buffer =  SCULL_B_BUFFER;

dev_t scull_b_devno;			/* Our first device number */

module_param(scull_b_nr_devs, int, 0);	/* FIXME check perms */
module_param(scull_b_buffer, int, 0);

static int scull_num_buffers = SCULL_NUM_ITEMS;
module_param(scull_num_buffers, int, 0);

static struct scull_bipe *scull_b_devices;

static int spacefree(struct scull_bipe *dev);
/*
 * Open and close
 */
static int scull_b_open(struct inode *inode, struct file *filp)
{
	struct scull_bipe *dev;

	dev = container_of(inode->i_cdev, struct scull_bipe, cdev);
	filp->private_data = dev;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;
	if (!dev->buffer) {
		/* allocate the buffer */
		PDEBUG("items : %d\n", SCULL_NUM_ITEMS);
		dev->buffer = kmalloc(sizeof(item_t) * SCULL_NUM_ITEMS, GFP_KERNEL);
		if (!dev->buffer) {
			up(&dev->sem);
			return -ENOMEM;
		}
	}
        PDEBUG("items : %d\n", SCULL_NUM_ITEMS);
	//dev->buffersize = scull_b_buffer;
	dev->end = dev->buffer + (SCULL_NUM_ITEMS-1) * sizeof(item_t);
 	if (!dev->rp && !dev->wp)
		dev->rp = dev->wp = dev->buffer; /* rd and wr from the beginning */

	/* use f_mode,not  f_flags: it's cleaner (fs/open.c tells why) */
	if (filp->f_mode & FMODE_READ) {
		dev->nreaders++;
                printk(KERN_ALERT, "Readers : %d",dev->nwriters); 
        }
	if (filp->f_mode & FMODE_WRITE) {
		dev->nwriters++;
		PDEBUG("------------WRITERS------ : %d",dev->nwriters);
	}
	up(&dev->sem);

	return nonseekable_open(inode, filp);
}

static int scull_b_release(struct inode *inode, struct file *filp)
{
	struct scull_bipe *dev = filp->private_data;
	PDEBUG("close called!");
	down(&dev->sem);
	if (filp->f_mode & FMODE_READ)
		dev->nreaders--;
	if (filp->f_mode & FMODE_WRITE)
		dev->nwriters--;
	if (dev->rp == dev->wp) {
		kfree(dev->buffer);
		dev->buffer = NULL; /* the other fields are not checked on open */
	}
	up(&dev->sem);
	return 0;
}

/*
 * Data management: read and write
*/
static ssize_t scull_b_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_bipe *dev = filp->private_data;

	if (down_interruptible(&dev->sem)) //mutex
		return -ERESTARTSYS;

	PDEBUG("\" (scull_b_read) dev->wp:%p    dev->rp:%p\" \n",dev->wp,dev->rp);

	while (dev->rp == dev->wp) { /* nothing to read */
		if (!dev->nwriters) {
			up(&dev->sem);
			return 0;
		}
		up(&dev->sem); /* release the lock */

		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		PDEBUG("\"%s\" reading: going to sleep\n", current->comm);
		if (wait_event_interruptible(dev->inq, (dev->rp != dev->wp)))
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		/* otherwise loop, but first reacquire the lock */
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}
	/* ok, data is there, return something */
	//if (dev->wp > dev->rp)
	//	count = min(count, (size_t)(dev->wp - dev->rp));
	//else /* the write pointer has wrapped, return data up to dev->end */
	//	count = min(count, (size_t)(dev->end - dev->rp));
	count = dev->rp->num_bytes;
	if (copy_to_user(buf, dev->rp, count)) {
		up (&dev->sem);
		return -EFAULT;
	}
	dev->rp += 1;
	if ((dev->rp - 1) == dev->end)
		dev->rp = dev->buffer; /* wrapped */
	up (&dev->sem);

	/* finally, awake any writers and return */
	wake_up_interruptible(&dev->outq);
	PDEBUG("\"%s\" did read %li bytes\n",current->comm, (long)count);
	return count;
}

/* Wait for space for writing; caller must hold device semaphore.  On
 * error the semaphore will be released before returning. */
static int scull_getwritespace(struct scull_bipe *dev, struct file *filp)
{
	while (spacefree(dev) == 0) { /* full */
		PDEBUG("Readers : %d", dev->nreaders);
		if (0 == dev->nreaders) {
			up(&dev->sem);
			return -1; //there are no consumers, return -1
		}

		DEFINE_WAIT(wait);
		
		up(&dev->sem);
		if (filp->f_flags & O_NONBLOCK)
			return -EAGAIN;
		PDEBUG("\"%s\" writing: going to sleep\n",current->comm);
		prepare_to_wait(&dev->outq, &wait, TASK_INTERRUPTIBLE);
		if (spacefree(dev) == 0 ) {
                        if (!(&dev->nreaders)) 
 				return -1;
			schedule();
		}
		finish_wait(&dev->outq, &wait);
		if (signal_pending(current))
			return -ERESTARTSYS; /* signal: tell the fs layer to handle it */
		if (down_interruptible(&dev->sem))
			return -ERESTARTSYS;
	}
	return 0;
}	

/* How much space is free? */
static int spacefree(struct scull_bipe *dev)
{
	if (dev->rp == dev->wp)
		return SCULL_NUM_ITEMS - 1;
	return ((dev->rp + SCULL_NUM_ITEMS - dev->wp) % SCULL_NUM_ITEMS) - 1;
}

static ssize_t scull_b_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
	struct scull_bipe *dev = filp->private_data;
	int result;

	if (down_interruptible(&dev->sem))
		return -ERESTARTSYS;

	/* Make sure there's space to write */
	result = scull_getwritespace(dev, filp);
	if (result == -1)  //val zero means success
		return 0; /* scull_getwritespace called up(&dev->sem) */
	else if (result)
		return result;

	/* ok, space is there, accept something */
	//count = min(count, (size_t)spacefree(dev));
	//if (dev->wp >= dev->rp)
	//	count = min(count, (size_t)(dev->end - dev->wp)); /* to end-of-buf */
	//else /* the write pointer has wrapped, fill up to rp-1 */
	//	count = min(count, (size_t)(dev->rp - dev->wp - 1));
	PDEBUG("Going to accept %li bytes to %p from %p\n", (long)count, dev->wp, buf);
	if (copy_from_user(dev->wp->buffer, buf, count)) {
		up (&dev->sem);
		return -EFAULT;
	}
	dev->wp->num_bytes = count;

	dev->wp += 1;
	if ((dev->wp - 1) == dev->end)
		dev->wp = dev->buffer; /* wrapped */
	PDEBUG("\" (scull_b_write) dev->wp:%p    dev->rp:%p\" \n",dev->wp,dev->rp);
	up(&dev->sem);

	/* finally, awake any reader */
	wake_up_interruptible(&dev->inq);  /* blocked in read() and select() */

	PDEBUG("\"%s\" did write %li bytes\n",current->comm, (long)count);
	return count;
}

static unsigned int scull_b_poll(struct file *filp, poll_table *wait)
{
	struct scull_bipe *dev = filp->private_data;
	unsigned int mask = 0;

	/*
	 * The buffer is circular; it is considered full
	 * if "wp" is right behind "rp" and empty if the
	 * two are equal.
	 */
	down(&dev->sem);
	poll_wait(filp, &dev->inq,  wait);
	poll_wait(filp, &dev->outq, wait);
	if (dev->rp != dev->wp)
		mask |= POLLIN | POLLRDNORM;	/* readable */
	if (spacefree(dev))
		mask |= POLLOUT | POLLWRNORM;	/* writable */
	up(&dev->sem);
	return mask;
}

/*
 * The file operations for the pipe device
 * (some are overlayed with bare scull)
 */
struct file_operations scull_bipe_fops = {
	.owner =	THIS_MODULE,
	.llseek =	no_llseek,
	.read =		scull_b_read,
	.write =	scull_b_write,
	.poll =		scull_b_poll,
	.open =		scull_b_open,
	.release =	scull_b_release,
};

/*
 * Set up a cdev entry.
 */
static void scull_b_setup_cdev(struct scull_bipe *dev, int index)
{
	int err, devno = scull_b_devno + index;
    
	cdev_init(&dev->cdev, &scull_bipe_fops);
	dev->cdev.owner = THIS_MODULE;
	err = cdev_add (&dev->cdev, devno, 1);
	/* Fail gracefully if need be */
	if (err)
		printk(KERN_NOTICE "Error %d adding scullpipe%d", err, index);
}

/*
 * Initialize the pipe devs; return how many we did.
 */
int scull_b_init(dev_t firstdev)
{
	int i, result;

	result = register_chrdev_region(firstdev, scull_b_nr_devs, "scullp");
	if (result < 0) {
		printk(KERN_NOTICE "Unable to get scullp region, error %d\n", result);
		return 0;
	}
	scull_b_devno = firstdev;
	scull_b_devices = kmalloc(scull_b_nr_devs * sizeof(struct scull_bipe), GFP_KERNEL);
	if (scull_b_devices == NULL) {
		unregister_chrdev_region(firstdev, scull_b_nr_devs);
		return 0;
	}
	memset(scull_b_devices, 0, scull_b_nr_devs * sizeof(struct scull_bipe));
	for (i = 0; i < scull_b_nr_devs; i++) {
		init_waitqueue_head(&(scull_b_devices[i].inq));
		init_waitqueue_head(&(scull_b_devices[i].outq));
		init_MUTEX(&scull_b_devices[i].sem);
		scull_b_setup_cdev(scull_b_devices + i, i);
	}
	return scull_b_nr_devs;
}

/*
 * This is called by cleanup_module or on failure.
 * It is required to never fail, even if nothing was initialized first
 */
void scull_b_cleanup(void)
{
	int i;

	if (!scull_b_devices)
		return; /* nothing else to release */

	for (i = 0; i < scull_b_nr_devs; i++) {
		cdev_del(&scull_b_devices[i].cdev);
		kfree(scull_b_devices[i].buffer);
	}
	kfree(scull_b_devices);
	unregister_chrdev_region(scull_b_devno, scull_b_nr_devs);
	scull_b_devices = NULL; /* pedantic */
}


