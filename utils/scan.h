#ifndef PHFS_SCAN_H
#define PHFS_SCAN_H

#if defined(__KERNEL__)
#  include <linux/types.h>
#  include <linux/stddef.h>
#  include <linux/string.h>
#  define PACKED __packed
#else
#  include <stddef.h>
#  include <stdint.h>
#  include <string.h>
#  define PACKED __attribute__((packed))
#endif

#define NOTFOUND      (-0x1000)
#define MAX_BODY      (2048u)

#define MAGIC0 0x13u
#define MAGIC1 0x37u
#define MAGIC2 0x63u   /* 'c' */
#define MAGIC3 0x75u   /* 'u' */

struct frame
{
	uint8_t  magic[4];
	uint8_t  op;
	uint8_t  seq;
	uint16_t blen;
	uint64_t arg;
} PACKED;

enum op
{
	OP_PING  = 0x01,     /*    return sizeof(frame)      */
	OP_ECHO  = 0x02,    /* validate XOR in (uint8_t)arg */
	OP_STATS = 0x03,   /*  write() return hits & 0xff  */
};

static inline long scan_ptr (const char *base, long back, long fwd,
                             const void *needle)
{
	long off;

	for (off = -back; off <= fwd; off += (long)sizeof(void *))
  {
      const void *cand;

      memcpy(&cand, base + off, sizeof(cand));
      if (cand == needle)
        return off;
	}
	return NOTFOUND;
}

static inline int fcheck (const struct frame *f, size_t avail,
                              size_t max_body)
{
	if (avail < sizeof(struct frame))
		return -1;
	if (f->magic[0] != MAGIC0 || f->magic[1] != MAGIC1 ||
	    f->magic[2] != MAGIC2 || f->magic[3] != MAGIC3)
		return -2;
	if (f->blen > max_body)
		return -3;
	return 0;
}

static inline unsigned char pxor (const unsigned char *b, size_t n)
{
	unsigned char x = 0;
	size_t 		  i;

	for (i = 0; i < n; i++)
		x ^= b[i];
	return x;
}

#endif /* PHFS_SCAN_H */
