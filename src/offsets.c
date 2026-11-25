#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/namei.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/dcache.h>
#include <linux/seq_file.h>
#include <linux/stddef.h>
#include <linux/magic.h>

#include "phfs.h"

long   delta_pde;
size_t off_ops;

static int probe_open (struct inode *inode,
                       struct file *file)
{
	return 0;
}

static struct proc_ops probe_ops = { .proc_open = probe_open, };

int probe_offsets (void)
{
	struct proc_dir_entry *pde;
	struct path p;
	long     a, b;

	if (delta_pde || off_ops)
		return 0;

	pde = proc_create(PHFS_PROBE_NAME, 0444, NULL, &probe_ops);
	if (!pde)
		return -ENOMEM;

	if (kern_path("/proc/" PHFS_PROBE_NAME, LOOKUP_FOLLOW, &p))
    {
  		remove_proc_entry(PHFS_PROBE_NAME, NULL);
  		return -ENOENT;
  	}

	a = scan_ptr((const char *)d_inode(p.dentry), SCAN_BACK,
			SCAN_FWD, pde);
	b = scan_ptr((const char *)pde, SCAN_BACK, OPS_FWD, &probe_ops);

	path_put(&p);
	remove_proc_entry(PHFS_PROBE_NAME, NULL);

	if (a == NOTFOUND || b == NOTFOUND)
		return -ESRCH;

	delta_pde = a;
	off_ops = (size_t)b;
	return 0;
}

static bool ops_plausible (const struct proc_ops *ops)
{
	static const unsigned int known = PROC_ENTRY_PERMANENT |
					  PROC_ENTRY_proc_read_iter |
					  PROC_ENTRY_proc_compat_ioctl |
					  PROC_ENTRY_proc_lseek;

	if (ops->proc_flags & ~known)
		return false;
	if (!ops->proc_open)
		return false;
	if (!ops->proc_read && !ops->proc_read_iter)
		return false;

	return true;
}

static bool ops_is_seq_style (const struct proc_ops *ops)
{
	return ops->proc_read      == seq_read      ||
	       ops->proc_read_iter == seq_read_iter ||
	       ops->proc_lseek     == seq_lseek     ||
	       ops->proc_release   == seq_release;
}

int resolve (const char *path, struct proc_dir_entry **pde_out,
	           const struct proc_ops **ops_out, bool seq_expected)
{
	struct path            p;
	struct proc_dir_entry *pde;
	const struct proc_ops *ops;
	int ret = -EINVAL;

	if (!delta_pde || !off_ops)
		return -EPROTO;

	if (kern_path(path, LOOKUP_FOLLOW, &p))
		return -ENOENT;

	if (!d_inode(p.dentry) || d_inode(p.dentry)->i_sb->s_magic != PROC_SUPER_MAGIC)
		goto out;

	pde = *(void **)((char *)d_inode(p.dentry) + delta_pde);
  if (!pde)
    {
      ret = -ENOTDIR;
      goto out;
    }

	ops = *(const struct proc_ops **)((char *)pde + off_ops);
	if (!ops || !ops_plausible(ops))
    {
      ret = -EPROTO;
      goto out;
    }

	if (ops->proc_write)
    {
      ret = -EEXIST;
      goto out;
    }

	if (seq_expected && !ops_is_seq_style(ops))
    {
  		pr_warn("%s no seq signature [ops=%px read=%px lseek=%px]\n",
  			path, ops, ops->proc_read, ops->proc_lseek);
  		ret = -EPROTO;
  		goto out;
  	}

	*pde_out = pde;
	*ops_out = ops;
	ret = 0;
out:
	path_put(&p);
	return ret;
}
