#ifndef _ONESHOT_UID
#define _ONESHOT_UID

#include <linux/spinlock.h>

#define RULE_STANDBY_UID "fw_standby_uid"

#define ONESHOT_UID_FIND_NONE 0
#define ONESHOT_UID_FIND_UIDCHAIN 1
#define ONESHOT_UID_FINE_END 2

struct oneshot_uid {
	u64 myrule_offset;
	void *myfilter_table;

	rwlock_t lock;
	struct list_head map_list;
};

struct oneshot_uid_map {
	u64 user_id;
	u64 *map;
	struct list_head list;
};
extern struct oneshot_uid oneshot_uid_ipv4;
extern struct oneshot_uid oneshot_uid_ipv6;
extern int oneshot_uid_checkmap(struct oneshot_uid *oneshot_uid_net,
				const struct sk_buff *skb,
				struct xt_action_param *par);
extern void oneshot_uid_resetmap(struct oneshot_uid *oneshot_uid_net);
extern int oneshot_uid_addrule_to_map(struct oneshot_uid *oneshot_uid_net,
				       const void *data);
extern void oneshot_uid_cleanup_unusedmem(struct oneshot_uid *oneshot_uid_net);

#endif /* _ONESHOT_UID */