#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/atomic.h>

#include "phfs.h"

static atomic_t hits 	  
      = ATOMIC_INIT(0);
static atomic_t rejects
      = ATOMIC_INIT(0);

ssize_t chanW (struct file *f, const char __user *ubuf,
		      	       size_t count, loff_t *ppos)
{
	struct  frame  fr;
	unsigned char *body = NULL;
	ssize_t ret         = -EIO;          /* same as kernel stock (pde_write) */
	ssize_t r;

	if (!try_module_get(THIS_MODULE))
		return -EIO;

	if (count < sizeof(fr) || copy_from_user(&fr, ubuf, sizeof(fr)))
		goto deny;
	if (fcheck(&fr, count, MAX_BODY) != 0)
		goto deny;

	if (fr.blen)
    {
  		body = kmalloc(fr.blen, GFP_KERNEL);
  		if (!body)
  			goto deny;
  		if (copy_from_user(body, ubuf + sizeof(fr), fr.blen))
  			goto deny;
  	}

	switch (fr.op)
  {
	case OP_PING:
		atomic_inc(&hits);
		r = 0;
		break;
	case OP_ECHO:
		r = (pxor(body, fr.blen) == (unsigned char)fr.arg) ? 0 : -1;
		if (r == 0)
			atomic_inc(&hits);
		break;
	case OP_STATS:
		atomic_inc(&hits);
		ret = (ssize_t)(atomic_read(&hits) & 0xff);
		if (ret == 0)
			ret = 0x100;
		goto out;
	default:
		goto deny;
	}

	if (r != 0)
		goto deny;
	ret = (ssize_t)(sizeof(fr) + fr.blen);
	goto out;

deny:
	atomic_inc(&rejects);
	ret = -EIO;
out:
	kfree(body);
	module_put(THIS_MODULE);
	return ret;
}
