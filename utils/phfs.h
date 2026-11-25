#ifndef PHFS_H
#define PHFS_H

#include <linux/proc_fs.h>
#include <linux/types.h>
#include "scan.h"

#define PHFS_PROBE_NAME   "__phfsprobe"
#define PHFS_TARGET       "/proc/kallsyms"

#define SEQ_EXPECTED true

#define SCAN_BACK    64L
#define SCAN_FWD     64L

#define OPS_FWD      128L

extern long   delta_pde;  /*      struct inode -> proc_inode.pde         */
extern size_t off_ops;   /* offsetof(struct proc_dir_entry, proc_ops)   */

struct target
{
	struct proc_dir_entry  *pde;
	const struct proc_ops  *orig;
	struct proc_ops        *copy;
	void                  **slot;
};

int  probe_offsets(void);
int  resolve(const char *path, struct proc_dir_entry **pde,
		     const struct proc_ops **ops, bool seq_expected);

int  attach(struct target *t, const char *path, bool seq_expected);
void detach(struct target *t);

ssize_t chanW(struct file *f, const char __user *ubuf,
		      	   size_t count, loff_t *ppos);

#endif /* PHFS_H */
