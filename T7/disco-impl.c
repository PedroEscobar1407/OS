/* Necessary includes for device drivers */
#include <linux/init.h>
/* #include <linux/config.h> */
#include <linux/module.h>
#include <linux/kernel.h> /* printk() */
#include <linux/slab.h> /* kmalloc() */
#include <linux/fs.h> /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/uaccess.h> /* copy_from/to_user */

#include "kmutex.h"

MODULE_LICENSE("Dual BSD/GPL");
#define TRUE 1
#define FALSE 0

/*Declaraciones*/
int dsk_open(struct inode *inode, struct file *flip);
int dsk_release(struct inode *inode, struct file *flip);
ssize_t dsk_read(struct file *filp, char *buf, size_t count, loff_t *f_pos);
ssize_t dsk_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos);
void dsk_exit(void);
int dsk_init(void);

struct file_operations dsk_fops = {
  read: dsk_read,
  write: dsk_write,
  open: dsk_open,
  release: dsk_release
};

module_init(dsk_init);
module_exit(dsk_exit);



int dsk_major = 61;


#define MAX_SIZE 8192

static int pend;


static KMutex mutex;
static KCondition cond;


typedef struct {
    char *buffer;           
    int in, out, size;    
    int close_reader;       
    int writer;         
    int reader;             
} Dsk_pipe;


Dsk_pipe *pipe_actual;

int dsk_init(void){
    int rc;
    rc = register_chrdev(dsk_major,"syncread", &dsk_fops);
    if( rc < 0){
        return rc;
    }
    pipe_actual = NULL;
    pend = FALSE;
    m_init(&mutex);
    c_init(&cond);
    return 0;
}

void dsk_exit(void) {
    unregister_chrdev(dsk_major, "syncread");
}

int dsk_open(struct inode *inode, struct file *filp) {
    int rc = 0;
    m_lock(&mutex);

    Dsk_pipe *p;
    if(filp->f_mode & FMODE_WRITE){
        if(pend){
            p = pipe_actual;
            filp->private_data = (void *)p;
            pend = FALSE;
        }
        else {
            p = kmalloc(sizeof(Dsk_pipe), GFP_KERNEL);
            if(!p){
                rc = -ENOMEM;
                goto epilog;
            }
            p->buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
            if(!p->buffer){
                kfree(p);
                rc = -ENOMEM;
                goto epilog;
            }
            memset(p->buffer, 0, MAX_SIZE);
            p->in = 0;
            p->out = 0;
            p->size = 0;
            p->close_reader = FALSE;
            pipe_actual = p;
            pend = TRUE;
        }
        p->writer = TRUE;
        p->reader = TRUE;
        p->size = 0;
        filp->private_data = (void *)p;
        c_broadcast(&cond);
    }
    else if(filp->f_mode & FMODE_READ){
        if(pend){
            p = pipe_actual;
            filp->private_data = (void *)p;
            pend = FALSE;
            c_broadcast(&cond);
        }
        else {
            p = kmalloc(sizeof(Dsk_pipe), GFP_KERNEL);
            if(!p){
                rc = -ENOMEM;
                goto epilog;
            }
            p->buffer = kmalloc(MAX_SIZE, GFP_KERNEL);
            if(!p->buffer){
                kfree(p);
                rc = -ENOMEM;
                goto epilog;
            }
            memset(p->buffer, 0, MAX_SIZE);
            p->in = 0;
            p->out = 0;
            p->size = 0;
            p->close_reader = FALSE;
            p->writer = FALSE;
            p->reader = FALSE;
            pipe_actual = p;
            pend = TRUE;
            filp->private_data = (void *)p;
        }
    }

epilog:
    m_unlock(&mutex);
    return rc;
}

int dsk_release(struct inode *inode, struct file *filp){
    m_lock(&mutex);
    Dsk_pipe *p = (Dsk_pipe *)filp->private_data;
    if(filp->f_mode & FMODE_WRITE){
        p->close_reader = TRUE;
        c_broadcast(&cond);
    }
    else if(filp->f_mode & FMODE_READ){
    }
    m_unlock(&mutex);
    return 0;
}





ssize_t dsk_read(struct file *filp, char *buf, size_t count, loff_t *f_pos) {
    m_lock(&mutex);
    Dsk_pipe *p = (Dsk_pipe *)filp->private_data;
    while (!p->writer || p->size <= *f_pos) {
        if (c_wait(&cond, &mutex)) {
            count = -EINTR;
            goto epilog;
        }
        if (p->close_reader) {
            count = 0;
            goto epilog;
        }
    }

    if (count > p->size) {
        count = p->size;
    }

    for (int k = 0; k < count; k++) {
        if (copy_to_user(buf + k, p->buffer + p->out, 1) != 0) {
            count = -EFAULT;
            goto epilog;
        }
        p->out = (p->out + 1) % MAX_SIZE;
        p->size--;
    }

epilog:
    m_unlock(&mutex);
    return count;
}

ssize_t dsk_write(struct file *filp, const char *buf, size_t count, loff_t *f_pos) {
    m_lock(&mutex);
    Dsk_pipe *p = (Dsk_pipe *)filp->private_data;
    for(int k=0; k < count; k++){
        while(p->size == MAX_SIZE){
            if(c_wait(&cond, &mutex)){
                m_unlock(&mutex);
                return -EINTR;
            }
        }

        if(copy_from_user(p->buffer + p->in, buf + k, 1) != 0){
            count = -EFAULT;
            goto epilog;
        }
        p->in = (p->in + 1) % MAX_SIZE;
        p->size++;
        c_broadcast(&cond);
    }

epilog:
    m_unlock(&mutex);
    return count;
}











