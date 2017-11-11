#include <net/sock.h>
#include <linux/skbuff.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_qtaguid.h>
#include <linux/file.h>
#include <linux/netfilter/xt_socket.h>
#include <net/netfilter/oneshot_uid.h>
#include <linux/spinlock.h>
#include <net/request_sock.h>
#include <net/inet_sock.h>

struct oneshot_uid oneshot_uid_ipv4;
struct oneshot_uid oneshot_uid_ipv6;

#define USERID_OFFSET 100000
#define UID_MINVAL_OFFSET 10000
#define UID_MAXSIZE 8192
#define RULEMAPSIZE 128
#define ONELINE 64

static struct sock *qtaguid_find_sk(const struct sk_buff *skb,
				    struct xt_action_param *par)
{
	struct sock *sk;
	unsigned int hook_mask = (1 << par->hooknum);

	/*
	 * Let's not abuse the the xt_socket_get*_sk(), or else it will
	 * return garbage SKs.
	 */
	if (!(hook_mask &
	      ((1 << NF_INET_PRE_ROUTING) | (1 << NF_INET_LOCAL_IN))))
		return NULL;

	switch (par->family) {
	case NFPROTO_IPV6:
		sk = xt_socket_lookup_slow_v6(dev_net(skb->dev), skb, par->in);
		break;
	case NFPROTO_IPV4:
		sk = xt_socket_lookup_slow_v4(dev_net(skb->dev), skb, par->in);
		break;
	default:
		return NULL;
	}

	if (sk)
		/*
		 * When in TCP_TIME_WAIT the sk is not a "struct sock" but
		 * "struct inet_timewait_sock" which is missing fields.
		 */
		if (!sk_fullsock(sk) || sk->sk_state  == TCP_TIME_WAIT) {
			if (sk->sk_state == TCP_NEW_SYN_RECV) {
				struct request_sock *req = (struct request_sock *)sk;
				if (req->rsk_listener) {
					atomic_inc(&req->rsk_listener->sk_refcnt);
					sock_gen_put(sk);
					return req->rsk_listener;
				}
			}
			sock_gen_put(sk);
			sk = NULL;
		}

	return sk;
}

#ifdef ONESHOT_UID_DEBUG
static void oneshot_uid_printrule(struct oneshot_uid *oneshot_uid_net)
{
	unsigned int i;
	struct oneshot_uid_map *map_pos;

	pr_err("%s begin\n", __func__);
	list_for_each_entry(map_pos, &oneshot_uid_net->map_list, list) {
		for (i = 0; i < RULEMAPSIZE; i++)
			if (map_pos->map[i]) {
				pr_err("%s user_id:%llu, i:%d value:%llu\n",
				       __func__, map_pos->user_id,
				       i, map_pos->map[i]);
			}
	}

	pr_err("%s end\n", __func__);
}
#endif

int oneshot_uid_checkmap(struct oneshot_uid *oneshot_uid_net,
			 const struct sk_buff *skb, struct xt_action_param *par)
{
	struct oneshot_uid_map *map_pos;
	const struct file *filp;
	u64 uid;
	kuid_t sock_uid;
	long idx;
	long bit;
	int ret = 0;
	bool set_sk_callback_lock = false;
	bool got_sock = false;
	struct sock *sk;
	u64 user_id;

	if (skb == NULL)
		return 0;

	sk = skb_to_full_sk(skb);
	if (sk && sk->sk_state == TCP_TIME_WAIT)
		sk = NULL;

	if (sk == NULL) {
		/*
		 * A missing sk->sk_socket happens when packets are in-flight
		 * and the matching socket is already closed and gone.
		 */
		sk = qtaguid_find_sk(skb, par);
		got_sock = sk;
	}

	if (sk != NULL) {
		set_sk_callback_lock = true;
		read_lock_bh(&sk->sk_callback_lock);
		filp = sk->sk_socket ? sk->sk_socket->file : NULL;
	}

	if (sk == NULL || sk->sk_socket == NULL)
		goto put_sock_ret_res;

	filp = sk->sk_socket->file;
	if (filp == NULL) {
		ret = 0;
		goto put_sock_ret_res;
	}

	sock_uid = filp->f_cred->fsuid;
	uid = sock_uid.val;

	user_id = uid / USERID_OFFSET;
	uid %= USERID_OFFSET;

	if (uid < UID_MINVAL_OFFSET || uid >= UID_MAXSIZE + UID_MINVAL_OFFSET) {
		ret = 0;
		goto put_sock_ret_res;
	}

	uid -= UID_MINVAL_OFFSET;

	idx = uid / ONELINE;
	bit = uid % ONELINE;

	list_for_each_entry(map_pos, &oneshot_uid_net->map_list, list) {
		if (user_id == map_pos->user_id &&
		    ((map_pos->map[idx]) & (1LL << bit))) {
			ret = 1;
			goto put_sock_ret_res;
		}
	}

put_sock_ret_res:
	if (got_sock)
		sock_gen_put(sk);
	if (set_sk_callback_lock)
		read_unlock_bh(&sk->sk_callback_lock);

	return ret;
}
EXPORT_SYMBOL(oneshot_uid_checkmap);

void oneshot_uid_resetmap(struct oneshot_uid *oneshot_uid_net)
{
	struct oneshot_uid_map *map_pos;

	list_for_each_entry(map_pos, &oneshot_uid_net->map_list, list) {
		map_pos->user_id = -1;
		memset(map_pos->map, 0, sizeof(u64) * RULEMAPSIZE);
	}
}
EXPORT_SYMBOL(oneshot_uid_resetmap);

int oneshot_uid_addrule_to_map(struct oneshot_uid *oneshot_uid_net,
				const void *data)
{
	const struct xt_qtaguid_match_info *info = data;
	struct oneshot_uid_map *map_pos;

	if (info->match & XT_QTAGUID_UID) {
		kuid_t uid_min = make_kuid(&init_user_ns, info->uid_min);

		/* not support range rule */
		/* kuid_t uid_max = make_kuid(&init_user_ns, info->uid_max); */

		u64 idx, bit;
		u64 uid_val = uid_min.val;
		u64 user_id = uid_val / USERID_OFFSET;

		uid_val %= USERID_OFFSET;

		if (uid_val < UID_MINVAL_OFFSET ||
		    uid_val >= UID_MAXSIZE + UID_MINVAL_OFFSET) {
#ifdef ONESHOT_UID_DEBUG
			/* error only support 10000 ~ 1XXXX uid */
			pr_err("%s unsupported user_id:%llu, uid num:%llu\n",
			       __func__, user_id, uid_val);
#endif
			return -EBADF;
		}

		/* over than 10000 */
		uid_val -= UID_MINVAL_OFFSET;

		idx = uid_val / ONELINE;
		bit = uid_val % ONELINE;

#ifdef ONESHOT_UID_DEBUG
		pr_err
		    ("%s addrule user_id:%llu, uid:%llu, idx:%lld, bit:%lld\n",
		     __func__, user_id, uid_val, idx, bit);
#endif
		list_for_each_entry(map_pos, &oneshot_uid_net->map_list, list) {
			if (user_id == map_pos->user_id && map_pos->map) {
				map_pos->map[idx] |= (1LL << bit);
#ifdef ONESHOT_UID_DEBUG
				oneshot_uid_printrule(oneshot_uid_net);
#endif
				return 0;
			}
		}

		/* alloc new oneshot_uid_chmap */
		map_pos = kmalloc(sizeof(*map_pos), GFP_ATOMIC);
		if (!map_pos)
			return -ENOMEM;
		map_pos->user_id = user_id;
		map_pos->map = kmalloc(sizeof(u64) * RULEMAPSIZE, GFP_ATOMIC);
		if (!map_pos->map) {
			kfree(map_pos);
			return -ENOMEM;
		}
		memset(map_pos->map, 0, sizeof(u64) * RULEMAPSIZE);
		map_pos->map[idx] |= (1LL << bit);

		list_add_tail(&map_pos->list, &oneshot_uid_net->map_list);
#ifdef ONESHOT_UID_DEBUG
		oneshot_uid_printrule(oneshot_uid_net);
#endif
	}
	
	return 0;
}
EXPORT_SYMBOL(oneshot_uid_addrule_to_map);

void oneshot_uid_cleanup_unusedmem(struct oneshot_uid *oneshot_uid_net)
{
	/* clean up unused memorymap */
	struct oneshot_uid_map *map_pos, *oneshot_uid_map_pos2;

	list_for_each_entry_safe(map_pos, oneshot_uid_map_pos2,
				 &oneshot_uid_net->map_list, list) {
		if (map_pos->user_id == -1) {
			kfree(map_pos->map);
			list_del(&map_pos->list);
			kfree(map_pos);
		}
	}
}
EXPORT_SYMBOL(oneshot_uid_cleanup_unusedmem);

static int __init oneshot_uid_init(void)
{
	INIT_LIST_HEAD(&oneshot_uid_ipv4.map_list);
	INIT_LIST_HEAD(&oneshot_uid_ipv6.map_list);

	rwlock_init(&oneshot_uid_ipv4.lock);
	rwlock_init(&oneshot_uid_ipv6.lock);
	return 0;
}

static void __exit oneshot_uid_fini(void)
{
	oneshot_uid_resetmap(&oneshot_uid_ipv4);
	oneshot_uid_resetmap(&oneshot_uid_ipv6);

	oneshot_uid_cleanup_unusedmem(&oneshot_uid_ipv4);
	oneshot_uid_cleanup_unusedmem(&oneshot_uid_ipv6);
}

module_init(oneshot_uid_init);
module_exit(oneshot_uid_fini);