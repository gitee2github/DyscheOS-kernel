#ifndef __DYSCHE_S___
#define __DYSCHE_S___

bool is_dysche_mode(void);

/* max partition count */
#define	PARTITION_MAXCNT	4
#define PARTITION_VALID(id) ((id) >= 0 && (id) < PARTITION_MAXCNT)

typedef u32 dysche_id;

dysche_id get_dysche_id(void);


#endif