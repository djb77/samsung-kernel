
#define pr_fmt(fmt) "mmiotrace : " fmt

#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/aio.h>
#include <linux/sec_debug.h>
#include <linux/hw_breakpoint.h>
#include <linux/kthread.h>
#include <linux/sec_sysfs.h>
#include <linux/smp.h>
#include <linux/sec_mmiotrace.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_platform.h>

#ifdef CONFIG_NO_BOOTMEM
#include <linux/memblock.h>
#endif

#include <asm/debug-monitors.h>
#include <asm/system_misc.h>
#include <asm/smp_plat.h>
#include <asm/cacheflush.h>
#include <asm/map.h>
#include <asm/tlbflush.h>
#include <asm/stacktrace.h>
#include <asm-generic/div64.h>
#include <asm/esr.h>

static LIST_HEAD(mmiotrace_region);
static LIST_HEAD(mmiotrace_ioremap_list);

static DEFINE_PER_CPU(struct mmiotrace_faultinfo, pcpu_faultinfo);
static DEFINE_SPINLOCK(mmiotrace_lock);
static DEFINE_SPINLOCK(mmiotrace_region_lock);
static DEFINE_SPINLOCK(mmiotrace_ioremap_lock);

/* Currently stepping a per-CPU kernel breakpoint. */
DECLARE_PER_CPU(int, stepping_kernel_bp);

#if PERFORMANCE_PROFILE
static DEFINE_PER_CPU(int, pcpu_profinfo);
#endif

static char *mmio_cmdline;

static struct mmiotrace_loginfo mmiotrace_log_buf[MMIOTRACE_LOG_BUFFER_SIZE];
#if PERFORMANCE_PROFILE
static struct mmiotrace_profinfo mmiotrace_prof_buf[MMIOTRACE_PROF_BUFFER_SIZE];
#endif
/* cur_loginfo must be protected by mmiotrace_lock */
static int cur_loginfo;
#if PERFORMANCE_PROFILE
static int cur_profinfo;
#endif
static bool mmiotrace_enable;

static int mmiotrace_set_mem(unsigned long addr, unsigned long size, bool en)
{
	unsigned long start = addr & PAGE_MASK;
	unsigned long end = start + size - 1;
	int ret = -EINVAL, numpages = virt_to_pfn(end) - virt_to_pfn(start) + 1;

	if (!addr)
		return ret;

	if (en)
		ret = set_memory_valid_n(start, numpages);
	else if (!en && mmiotrace_enable)
		ret = set_memory_invalid_n(start, numpages);

	if (ret) {
		if (!mmiotrace_enable)
			pr_debug("enable is not set. (0x%lx)\n", addr);
		else
			pr_err("failed to set 0x%lx(%d)\n", addr, ret);
	} else
		pr_debug("0x%lx(0x%lx) is set\n", addr, size);

	return ret;
}

unsigned long mmiotrace_virt_to_phys(unsigned long virt_addr)
{
	unsigned long phys_addr = 0;
	unsigned long offset = 0;
	struct mmiotrace_addrinfo *tmp;
	unsigned long region_flags;

	spin_lock_irqsave(&mmiotrace_region_lock, region_flags);
	list_for_each_entry(tmp, &mmiotrace_region, list) {
		unsigned long start = tmp->virt_addr;
		unsigned long end = start + tmp->size - 1;

		if (start <= virt_addr && virt_addr <= end) {
			offset = (virt_addr & 0xffff) - (start & 0xffff);
			phys_addr = tmp->phys_addr + offset;

			spin_unlock_irqrestore(&mmiotrace_region_lock,
					       region_flags);

			return phys_addr;
		}
	}

	spin_unlock_irqrestore(&mmiotrace_region_lock, region_flags);
	pr_err("Error get phys_addr\n");
	return phys_addr;
}

/*
 * mmiotrace_is_trace_region()

 * description.
 * check if this fault is caused by mem access to a region
 * under consideration of mmiotrace

 * param def.
 *	regs :
 *	esr :
*/
static int mmiotrace_is_trace_region(unsigned long virt_addr, unsigned int esr,
				     struct mmiotrace_faultinfo *finfo)
{
	struct mmiotrace_addrinfo *tmp, *data;
	unsigned long region_flags;

	finfo->state = MMIOTRACE_STATE_NONE;

	spin_lock_irqsave(&mmiotrace_region_lock, region_flags);

	list_for_each_entry_safe(tmp, data, &mmiotrace_region, list) {
		unsigned long start = tmp->virt_addr;
		unsigned long end = start + tmp->size - 1;

		if (!start)
			continue;

		if (virt_to_page(start) <= virt_to_page(virt_addr) &&
		    virt_to_page(virt_addr) <= virt_to_page(end)) {
			finfo->addr = virt_addr;
			finfo->size = 0x4;
			finfo->type = tmp->type;
			finfo->state = MMIOTRACE_STATE_PAGE;
			if (start <= virt_addr && virt_addr <= end) {
				finfo->state = MMIOTRACE_STATE_RANGE;
				finfo->size = tmp->size;
				finfo->type = tmp->type;
				pr_debug("hit_addr=0x%llx size=0x%llx\n",
					 finfo->addr, finfo->size);
				list_del(&tmp->list);
				list_add(&tmp->list, &mmiotrace_region);
			goto out;
			}
		}
	}
out:
	spin_unlock_irqrestore(&mmiotrace_region_lock, region_flags);
	return finfo->state ? 0 : -EFAULT;
}

static unsigned long change_format_f(u64 ts)
{
	unsigned long rem_nsec;

	if (!ts)
		return 0;

	rem_nsec = do_div(ts, 1000000000);
	return ts;
}

static unsigned long change_format_s(u64 ts)
{
	unsigned long rem_nsec;

	if (!ts)
		return 0;

	rem_nsec = do_div(ts, 1000000000);
	return rem_nsec / 1000;
}

#if SAVE_CALLSTACK
static void mmiotrace_callstack(int index)
{
	struct stackframe frame;
	int cnt = 0;

	frame.fp = (unsigned long)__builtin_frame_address(0);
	frame.sp = current_stack_pointer;
	frame.pc = (unsigned long)mmiotrace_callstack;

	while (cnt < 4) {
		unsigned long where = frame.pc;
		int ret;

		ret = unwind_frame(&frame);
		if (ret < 0)
			break;

		mmiotrace_log_buf[index].callstack[cnt++] = where;
	}
}
#endif

static u32 mmiotrace_get_evt_type(unsigned int esr)
{
	int write = ((esr & ESR_EL1_WRITE) >> ESR_EL1_WRITE_SHIFT);

	if (write)
		return MMIOTRACE_TYPE_STORE;
	else
		return MMIOTRACE_TYPE_LOAD;
}

/*
 * mmiotrace_do_fault_handler()

 * description.
 * this function allows a cpu to access mmio region of 'addr'
 * by changing permions of the region from 'no access' to 'read/write'

 * param def.
 *	regs :
 *	esr :

 * return def.
 * 1 : this region is under consideration of mmio tracer
 * 0 : this region has nothing to do with mmio tracer
*/
int mmiotrace_do_fault_handler(unsigned long virt_addr,
			       unsigned int esr, struct pt_regs *regs)
{
	struct mmiotrace_faultinfo *finfo, pfinfo;
	int ret = 0;
	int *kernel_step;
#if PERFORMANCE_PROFILE
	u64 temp;
	int *profinfo;

	temp = local_clock();
#endif

	if (!mmiotrace_is_trace_region(virt_addr, esr, &pfinfo)) {
		if (!mmiotrace_enable)
			return !ret;

		local_irq_disable();
		spin_lock(&mmiotrace_lock);
#if PERFORMANCE_PROFILE
		cur_profinfo++;
		cur_profinfo %= MMIOTRACE_PROF_BUFFER_SIZE;

		profinfo = this_cpu_ptr(&pcpu_profinfo);
		*profinfo = cur_profinfo;

		mmiotrace_prof_buf[cur_profinfo].cur_loginfo = -1;
		mmiotrace_prof_buf[cur_profinfo].t[0] = temp;
		mmiotrace_prof_buf[cur_profinfo].t[1] = local_clock();
#endif
		finfo = this_cpu_ptr(&pcpu_faultinfo);

		/* BUG on missing single step exception */
		if (finfo->fcnt != finfo->scnt) {
			u32 oslsr_el1;

			asm volatile ("mrs %0, oslsr_el1" : "=r" (oslsr_el1)::);
			pr_err("f=%llu s=%llu OSlock(%d)\n",
			       finfo->fcnt, finfo->scnt, oslsr_el1 & 0x2);
			WARN_ON(1);
		}

		finfo->fcnt++;
		finfo->addr = pfinfo.addr;
		finfo->state = pfinfo.state;
		finfo->size = pfinfo.size;
		finfo->type = pfinfo.type;
		finfo->esr = esr;

		/*
		 * change fault context's PSTATE.D & I temporarily to let single step exception
		 * take place right after executing memory access instruction.
		 * if PSTATE.D == 1 then change it to 0
		 * if PSTATE.I == 0 then change it to 1
		 */
		finfo->pstate_changed = 0;
		if((regs->pstate & MMIOTRACE_PSTATE_D) == MMIOTRACE_PSTATE_D
			|| (regs->pstate & MMIOTRACE_PSTATE_I) == 0) {
			finfo->pstate_changed = 1;
			finfo->saved_pstate = (regs->pstate & MMIOTRACE_SAVED_PSTATE_MASK);
			regs->pstate = (regs->pstate & ~MMIOTRACE_SAVED_PSTATE_MASK) | MMIOTRACE_PSTATE_TARGET;
		}

#if PERFORMANCE_PROFILE
		mmiotrace_prof_buf[cur_profinfo].t[2] = local_clock();
#endif
		mmiotrace_set_mem(finfo->addr, finfo->size, true);
#if PERFORMANCE_PROFILE
		mmiotrace_prof_buf[cur_profinfo].t[3] = local_clock();
#endif
		if (finfo->state & MMIOTRACE_STATE_RANGE) {
			pr_debug("+%pS (0x%lx)\n", (void *)(regs->pc),
				 virt_addr);
			pr_debug("PC = 0x%llx\n", regs->pc);
			pr_debug("PC Fn = %pS\n", (void *)(regs->pc));
			pr_debug("hit 0x%llx (0x%llx)\n",
				 finfo->addr, finfo->size);

			if (finfo->type & mmiotrace_get_evt_type(esr)) {
				/* finfo->index is assigned cur_loginfo */
				/* single_step_handler must use finfo->index */
				cur_loginfo++;
				cur_loginfo %= MMIOTRACE_LOG_BUFFER_SIZE;
				finfo->index = cur_loginfo;
#if PERFORMANCE_PROFILE
				mmiotrace_prof_buf[cur_profinfo].cur_loginfo =
				    cur_loginfo;
#endif
				mmiotrace_log_buf[cur_loginfo].ts_nsec =
				    local_clock();
				mmiotrace_log_buf[cur_loginfo].virt_addr =
				    finfo->addr;
				mmiotrace_log_buf[cur_loginfo].phys_addr =
				    mmiotrace_virt_to_phys(virt_addr);
				mmiotrace_log_buf[cur_loginfo].pc = regs->pc;
				mmiotrace_log_buf[cur_loginfo].loginfo =
				    mmiotrace_get_evt_type(esr);

				mmiotrace_log_buf[cur_loginfo].loginfo |=
				    smp_processor_id() << LOGINFO_CORE_SHIFT;
#if SAVE_CALLSTACK
				mmiotrace_callstack(cur_loginfo);
#endif
			}
		}
#if PERFORMANCE_PROFILE
		mmiotrace_prof_buf[cur_profinfo].t[4] = local_clock();
#endif
		/*
		 *      single_step
		 */
		local_dbg_disable();

		kernel_step = this_cpu_ptr(&stepping_kernel_bp);

		if (kernel_active_single_step())
			*kernel_step = ARM_KERNEL_STEP_SUSPEND;
		else
			kernel_enable_single_step(regs);

		ret = !!finfo->state;
#if PERFORMANCE_PROFILE
		mmiotrace_prof_buf[cur_profinfo].t[5] = local_clock();
#endif
		spin_unlock(&mmiotrace_lock);
	}

	return ret;
}

/*
 * mmiotrace_single_step_handler()
 * description.
 * In this handler, mmiotrace saves result of mmio access event and
 * sets the mmio region cpu accessed a bit ago.
 * Plus, in order to get result of mmio access event
 * we should decode an instruction cpu executed
 * param def.
 *	regs :
 *	esr :
*/
static int mmiotrace_single_step_handler(struct pt_regs *regs, unsigned int esr)
{
	int ret = DBG_HOOK_HANDLED;
	struct mmiotrace_faultinfo *finfo;
	unsigned long flags;
	unsigned int rt;
#if PERFORMANCE_PROFILE
	int *profinfo;
#endif

#if PERFORMANCE_PROFILE
	preempt_disable();
	profinfo = this_cpu_ptr(&pcpu_profinfo);
	preempt_enable();

	mmiotrace_prof_buf[*profinfo].t[6] = local_clock();
#endif
	spin_lock_irqsave(&mmiotrace_lock, flags);
#if PERFORMANCE_PROFILE
	mmiotrace_prof_buf[*profinfo].t[7] = local_clock();
#endif

	pr_debug("%s : PC = 0x%llx\n", __func__, regs->pc);
	pr_debug("%s : PC Fn = %pS\n", __func__, (void *)(regs->pc));

	finfo = this_cpu_ptr(&pcpu_faultinfo);

	kernel_disable_single_step();

	rt = (int)((*(u64 *)((regs->pc) - 4)) & 0x1F);

	/*
	 * check if single step is delayed due to a certain reason
	 */
	if (regs->pc != mmiotrace_log_buf[finfo->index].pc + 4) {

		pr_err("single step is delayed, fault:0x%lx(%pS), ss position:0x%lx(%pS) \n",
			(unsigned long)mmiotrace_log_buf[finfo->index].pc,
			(void *)(mmiotrace_log_buf[finfo->index].pc),
			(unsigned long)regs->pc, (void *)(regs->pc));
		WARN_ON(1);
	}

	if (finfo->state & MMIOTRACE_STATE_RANGE) {
		if (finfo->type & mmiotrace_get_evt_type(finfo->esr)) {
			mmiotrace_log_buf[finfo->index].io_data =
			    regs->regs[rt];

			pr_debug("data=0x%llx @ 0x%lx\n",
				 regs->regs[rt],
				 mmiotrace_virt_to_phys(finfo->addr));
		}
	}

	pr_debug("-%pS (0x%llx)\n\n", (void *)(regs->pc), finfo->addr);

#if PERFORMANCE_PROFILE
	mmiotrace_prof_buf[*profinfo].t[8] = local_clock();
#endif
	mmiotrace_set_mem(finfo->addr, finfo->size, false);
#if PERFORMANCE_PROFILE
	mmiotrace_prof_buf[*profinfo].t[9] = local_clock();
#endif
	finfo->state = MMIOTRACE_STATE_NONE;

	if (finfo->pstate_changed) {
		regs->pstate = (regs->pstate & ~MMIOTRACE_SAVED_PSTATE_MASK) | finfo->saved_pstate;
	}
	finfo->saved_pstate = 0;
	finfo->pstate_changed = 0;

	finfo->scnt++;
	spin_unlock_irqrestore(&mmiotrace_lock, flags);
#if PERFORMANCE_PROFILE
	preempt_disable();
	mmiotrace_prof_buf[*profinfo].t[10] = local_clock();
	preempt_enable();
#endif

	ret = DBG_HOOK_HANDLED;
	return ret;
}

static int mmiotrace_list_add(phys_addr_t phys_addr, unsigned long virt_addr,
			      unsigned long long size, u32 type, const char *name)
{
	struct mmiotrace_addrinfo *data, *tmp;
	int ret = 0;

	data = kzalloc(sizeof(struct mmiotrace_addrinfo), GFP_ATOMIC);
	if (unlikely(!data)) {
		pr_err("failed to set region 0x%llx\n", phys_addr);
		return -ENOMEM;
	}

	data->phys_addr = phys_addr;
	data->virt_addr = virt_addr;
	data->size = size;
	data->type = type;
	data->name = name;

	list_for_each_entry(tmp, &mmiotrace_region, list)
		if (virt_addr != 0 && tmp->virt_addr == virt_addr) {
			tmp->size = size;
			kfree(data->name);
			kfree(data);
			return MMIOTRACE_INVALID_PAGE;
		}

	list_add_tail(&data->list, &mmiotrace_region);
	pr_debug("0x%llx is added 0x%lx\n", phys_addr, virt_addr);

	return !!ret;
}

static void mmiotrace_list_del(unsigned long virt_addr, bool en)
{
	struct mmiotrace_addrinfo *data, *tmp;
	struct page *page = virt_to_page(virt_addr);
	int hit = 0;
	unsigned long region_flags;

	spin_lock_irqsave(&mmiotrace_region_lock, region_flags);
	list_for_each_entry_safe(data, tmp, &mmiotrace_region, list) {
		if (virt_to_page(data->virt_addr) == page) {
			hit++;
			if (data->virt_addr == virt_addr) {
				pr_debug("0x%llx is removed 0x%lx\n",
					 data->phys_addr, virt_addr);
				list_del(&data->list);
				kfree(data->name);
				kfree(data);
			}
		}
	}
	spin_unlock_irqrestore(&mmiotrace_region_lock, region_flags);

	if (hit && en == 1)
		mmiotrace_set_mem(virt_addr, data->size, true);
}

/*
 * sec_mmiotrace_commander_set()
 * param def.
 *	start :
 *	size :
 *	name :
 *	access_type 1:ARM_BREAKPOINT_LOAD,2:ARM_BREAKPOINT_STORE,3:BOTH
*/
void sec_mmiotrace_commander_set(unsigned long long phys_addr,
				 unsigned long long size, unsigned long type, const char *name)
{
	struct mmiotrace_ioremap_addr *ioremap_data, *tmp;
	unsigned long virt_addr = 0;
	unsigned long flags;
	unsigned long region_flags;
	unsigned long ioremap_flags;
	unsigned long long region_addr = phys_addr;
	unsigned long long region_size = 0;
	unsigned long long remainder = region_addr & (PAGE_SIZE - 1);
	bool target_found;

	spin_lock_irqsave(&mmiotrace_lock, flags);
	do {
		region_size = min(PAGE_SIZE - remainder, size);
		size -= region_size;
		remainder = 0;
		target_found = false;

		spin_lock_irqsave(&mmiotrace_ioremap_lock, ioremap_flags);
		list_for_each_entry_safe(ioremap_data, tmp,
					 &mmiotrace_ioremap_list, list) {
			if (region_addr < ioremap_data->phys_addr ||
			    ioremap_data->phys_addr + ioremap_data->size
					< region_addr)
				continue;

			virt_addr = ioremap_data->virt_addr;
			virt_addr += region_addr - ioremap_data->phys_addr;
			spin_lock_irqsave(&mmiotrace_region_lock, region_flags);
			target_found = true;
			if (!mmiotrace_list_add(region_addr, virt_addr,
						region_size, type, name))
				mmiotrace_set_mem(virt_addr, region_size,
						  false);
			spin_unlock_irqrestore(&mmiotrace_region_lock,
					       region_flags);
		}
		spin_unlock_irqrestore(&mmiotrace_ioremap_lock, ioremap_flags);

	/* target address is not found in ioremap_list, add it with vaddr 0 */
		if (!target_found) {
			spin_lock_irqsave(&mmiotrace_region_lock, region_flags);
			mmiotrace_list_add(region_addr, 0, region_size, type, name);
			spin_unlock_irqrestore(&mmiotrace_region_lock,
					       region_flags);
		}

		region_addr &= PAGE_MASK;
		region_addr += PAGE_SIZE;
	} while (size);

	spin_unlock_irqrestore(&mmiotrace_lock, flags);
}

static ssize_t sec_mmiotrace_trace_on_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t ssize)
{
	unsigned long flags;
	unsigned long region_flags;

	spin_lock_irqsave(&mmiotrace_lock, flags);

	if (!strtobool(buf, &mmiotrace_enable)) {
		struct mmiotrace_addrinfo *tdata;

		spin_lock_irqsave(&mmiotrace_region_lock, region_flags);
		list_for_each_entry(tdata, &mmiotrace_region, list)
			mmiotrace_set_mem(tdata->virt_addr,
					  tdata->size, !mmiotrace_enable);
		spin_unlock_irqrestore(&mmiotrace_region_lock, region_flags);
		pr_debug("mmiotrace is %sable\n",
			 mmiotrace_enable ? "en" : "dis");
	} else {
		pr_err("failed to set mmiotrace - input : %s\n", buf);
	}

	spin_unlock_irqrestore(&mmiotrace_lock, flags);
	return ssize;
}

static ssize_t sec_mmiotrace_trace_on_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	ssize_t ssize = 0;

	ssize = sprintf(buf, "mmiotrace is %sabled\n",
			mmiotrace_enable ? "en" : "dis");
	return ssize;
}

static ssize_t sec_mmiotrace_addr_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t ssize)
{
	/* TODO
	 * so dirty type casting
	 */
	char *str = (char *)buf;
	char *str1 = NULL, *str2 = NULL;
	unsigned long long phys_addr = memparse(str, &str1);
	unsigned long long size = memparse(str1 + 1, &str2);
	unsigned long long type = memparse(str2 + 1, &str2 + 1);

	if (phys_addr < MMIOTRACE_START_ADDR || MMIOTRACE_END_ADDR < phys_addr)
		pr_err("out of range phys_addr : 0x%llx\n", phys_addr);

	type = MMIOTRACE_TYPE_NONE;
	if (!strncmp(str2, " rw", 3))
		type = MMIOTRACE_TYPE_LOAD | MMIOTRACE_TYPE_STORE;
	else if (!strncmp(str2, " r", 2))
		type = MMIOTRACE_TYPE_LOAD;
	else if (!strncmp(str2, " w", 2))
		type = MMIOTRACE_TYPE_STORE;

	if (type == MMIOTRACE_TYPE_NONE) {
		pr_err("Usage : echo phys_addr size type(r/w/rw)\n");
		pr_err("Sample : echo 0x15100000 0x8 rw\n");
	} else {
		pr_info("phys_addr : 0x%llx size : 0x%llx type : (%s)(%llx)\n",
			phys_addr, size, str2, type);
		sec_mmiotrace_commander_set(phys_addr, size, type, NULL);
	}

	return ssize;
}

static ssize_t sec_mmiotrace_remove_store(struct device *dev,
					  struct device_attribute *attr,
					  const char *buf, size_t ssize)
{
	char *str = (char *)buf;
	char *str1 = NULL;
	int index = 1;
	struct mmiotrace_addrinfo *data, *tmp;
	unsigned long long remove_no = memparse(str, &str1);
	unsigned long flags;
	unsigned long region_flags;
	bool mmiotrace_enable_status;

	spin_lock_irqsave(&mmiotrace_lock, flags);

	spin_lock_irqsave(&mmiotrace_region_lock, region_flags);

	mmiotrace_enable_status = mmiotrace_enable;
	mmiotrace_enable = 0;

	if (!strncmp(str, "all", 3)) {
		pr_info("Remove all mmiotrace_region list\n");
		list_for_each_entry_safe(data, tmp, &mmiotrace_region, list) {
			index++;
			mmiotrace_set_mem(data->virt_addr, data->size, true);
			list_del(&data->list);
		}
		pr_info("Remove [%d] index list\n", index);
	} else {
		list_for_each_entry_safe(data, tmp, &mmiotrace_region, list) {
			if ((int)remove_no == (index++)) {
				pr_info("Remove phys 0x%llx   virt 0x%llx size : 0x%llx\n",
					data->phys_addr, data->virt_addr,
					data->size);

				mmiotrace_set_mem(data->virt_addr,
						  data->size, true);
				list_del(&data->list);

				mmiotrace_enable = mmiotrace_enable_status;
				spin_unlock_irqrestore(&mmiotrace_region_lock,
						       region_flags);
				spin_unlock_irqrestore(&mmiotrace_lock, flags);

				return ssize;
			}
		}
	}
	mmiotrace_enable = mmiotrace_enable_status;

	spin_unlock_irqrestore(&mmiotrace_region_lock, region_flags);

	spin_unlock_irqrestore(&mmiotrace_lock, flags);
	return ssize;
}

static ssize_t sec_mmiotrace_remove_show(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return 0;
}

static DEVICE_ATTR(trace_on, S_IRUGO | S_IWUSR | S_IWGRP,
		   sec_mmiotrace_trace_on_show, sec_mmiotrace_trace_on_store);

static DEVICE_ATTR(addr, S_IWUSR | S_IWGRP,
		   NULL, sec_mmiotrace_addr_store);

static DEVICE_ATTR(remove, S_IRUGO | S_IWUSR | S_IWGRP,
		   sec_mmiotrace_remove_show, sec_mmiotrace_remove_store);

static struct attribute *sec_mmiotrace_attrs[] = {
	&dev_attr_trace_on.attr,
	&dev_attr_addr.attr,
	&dev_attr_remove.attr,
	NULL,
};

static struct attribute_group sec_mmiotrace_attr_group = {
	.attrs = sec_mmiotrace_attrs,
};

void mmiotrace_add_ioremap(phys_addr_t phys_addr, unsigned long virt_addr,
			   size_t size)
{
	struct mmiotrace_ioremap_addr *data;
	unsigned long flags;
	unsigned long region_flags;
	unsigned long ioremap_flags;

	if (phys_addr < MMIOTRACE_START_ADDR || MMIOTRACE_END_ADDR < phys_addr)
		return;

	spin_lock_irqsave(&mmiotrace_lock, flags);

	data = kzalloc(sizeof(struct mmiotrace_ioremap_addr), GFP_ATOMIC);
	if (unlikely(!data)) {
		pr_err("failed to allocate\n");
	} else {
		struct mmiotrace_addrinfo *tdata, *tmp;

		data->phys_addr = phys_addr;
		data->virt_addr = virt_addr;
		data->size = size;
		spin_lock_irqsave(&mmiotrace_ioremap_lock, ioremap_flags);
		list_add_tail(&data->list, &mmiotrace_ioremap_list);
		spin_unlock_irqrestore(&mmiotrace_ioremap_lock, ioremap_flags);

		spin_lock_irqsave(&mmiotrace_region_lock, region_flags);
		list_for_each_entry_safe(tdata, tmp, &mmiotrace_region, list) {
			if (tdata->phys_addr != phys_addr)
				continue;
			if (!tdata->virt_addr) {
				tdata->virt_addr = virt_addr;
				tdata->size = size;
				kfree(data);
			} else if (!mmiotrace_list_add(phys_addr,
					virt_addr, tdata->size, tdata->type, NULL))
				mmiotrace_set_mem(virt_addr, tdata->size,
						  false);
		}
		spin_unlock_irqrestore(&mmiotrace_region_lock, region_flags);
	}

	spin_unlock_irqrestore(&mmiotrace_lock, flags);
}

void mmiotrace_remove_ioremap(unsigned long virt_addr)
{
	struct mmiotrace_ioremap_addr *data, *tmp;
	unsigned long flags;
	unsigned long ioremap_flags;

	spin_lock_irqsave(&mmiotrace_lock, flags);

	spin_lock_irqsave(&mmiotrace_ioremap_lock, ioremap_flags);
	list_for_each_entry_safe(data, tmp, &mmiotrace_ioremap_list, list) {
		if (data->virt_addr != virt_addr)
			continue;
		mmiotrace_list_del(virt_addr, false);
		list_del(&data->list);
		kfree(data);
	}
	spin_unlock_irqrestore(&mmiotrace_ioremap_lock, ioremap_flags);
	spin_unlock_irqrestore(&mmiotrace_lock, flags);
}

static void sec_mmiotrace_sysfs_init(void)
{
	struct device *dev;
	int ret = 0;

	dev = sec_device_create(NULL, "sec_mmiotrace");
	if (IS_ERR(dev)) {
		pr_err("err f%s,l%d\n", __func__, __LINE__);
		return;
	}
	ret = sysfs_create_group(&dev->kobj, &sec_mmiotrace_attr_group);
	if (ret < 0)
		pr_err("failed to create sysfs gruop\n");
}

static int parse_commandline(char *s)
{
/*
 *	format = baseaddr@size:type,baseaddr@size:type,...
 *	baseaddr : base of logging address
 *	size : size of address
 *	CAUTION : Using CMDLINE argument instantly enables trace_on feature
 *
 *	ex) CMDLINE = sec_mmiotrace=0x14C50000@0x1000:rw is equivalent to
 *	echo 0x14C50000 0x1000 rw > /sys/class/sec/sec_mmiotrace/addr
 *	and
 *	echo 1 > /sys/class/sec/sec_mmiotrace/trace_on
 */

	u64 start = 0, mem_size = 0, type = 0;

	if (!s)
		return -EINVAL;

	while (*s != '\n') {
		start = memparse(s, &s);
		if (*s == '@')
			mem_size = memparse(s + 1, &s);
		else
			break;
		if (!strncmp(s, ":rw", 3))
			type = MMIOTRACE_TYPE_LOAD | MMIOTRACE_TYPE_STORE;
		else if (!strncmp(s, ":r", 2))
			type = MMIOTRACE_TYPE_LOAD;
		else if (!strncmp(s, ":w", 2))
			type = MMIOTRACE_TYPE_STORE;
		else
			break;

		pr_debug("%llx:%llx:%llx\n", start, mem_size, type);
		sec_mmiotrace_commander_set(start, mem_size, type, NULL);
		if (*s == ',')
			s++;
	}

	sec_mmiotrace_trace_on_store(NULL, NULL, "1", 2);

	return 0;
}

static int __init sec_mmiotrace_setup(char *s)
{
	mmio_cmdline = s;
	return 0;
}

void sec_mmiotrace_stop(void)
{
	sec_mmiotrace_trace_on_store(NULL, NULL, "0", 2);
}

__setup("sec_mmiotrace=", sec_mmiotrace_setup);

static struct step_hook mmiotrace_step_hook = {
	.fn = mmiotrace_single_step_handler
};

static int sec_mmiotrace_init(void)
{
	mmiotrace_enable = false;
	/* init single step handler */
	register_step_hook(&mmiotrace_step_hook);

	sec_mmiotrace_sysfs_init();

	parse_commandline(mmio_cmdline);
	return 0;
}

static int sec_mmiotrace_addr_read(struct seq_file *file, void *p)
{
	struct mmiotrace_addrinfo *data;
	int index = 0;
	unsigned long region_flags;

	seq_printf(file, "%-16s%-20s%-10s%-22s%-10s\n",
		   "index", "phys addr", "type", "virt addr", "size");

	spin_lock_irqsave(&mmiotrace_region_lock, region_flags);
	list_for_each_entry(data, &mmiotrace_region, list)
		seq_printf(file, "%-16d0x%-18llx%-10d0x%-20llx0x%llx\t\t%s\n",
			   ++index, data->phys_addr, data->type,
			   data->virt_addr, data->size, data->name);
	spin_unlock_irqrestore(&mmiotrace_region_lock, region_flags);

	return 0;
}

static int sec_mmiotrace_addr_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, sec_mmiotrace_addr_read, NULL);
}

static const struct file_operations mmiotrace_addr_file_ops = {
	.owner = THIS_MODULE,
	.open = sec_mmiotrace_addr_open,
	.read = seq_read,
	.release = single_release,
	.llseek = seq_lseek,
};

static int sec_mmiotrace_read(struct seq_file *file, void *p)
{
	int start = 0, end = cur_loginfo;
	int i = 0;

	pr_debug("mmiotrace: cur_loginfo:%d\n", cur_loginfo);

	start = (cur_loginfo + 1) % MMIOTRACE_LOG_BUFFER_SIZE;

	if (!mmiotrace_log_buf[start].ts_nsec)
		start = 0;

#if 0				/* PERFORMANCE_PROFILE */
	seq_printf(file,
		   "\n%6s %-16s%-10s%-10s%-20s%-20s%-20s%-20s %-16s%-16s%-16s%-16s %-4s\n",
		   "#", "timestampe", "type", "ins name", "pc", "phys_addr",
		   "virt_addr", "io_data", "f_start", "f_end", "ss_start",
		   "ss_end", "core");
#else
	seq_printf(file, "\n%6s %-17s%-7s%-22s%-15s%-22s%-15s%-7s%-15s\n",
		   "#", "TIMESTAMP", "INST", "PC", "PHYS_ADDR",
		   "VIRT_ADDR", "IO_DATA", "CORE", "Fn");
#endif

	while (1) {
		struct mmiotrace_loginfo *info = mmiotrace_log_buf + start;

#if 0				/* PERFORMANCE_PROFILE */
		seq_printf(file,
			"%6d %-16lld%-10d%-10s0x%-18lx0x%-18lx0x%-18lx0x%-18lx %-16lld%-16lld%-16lld%-16lld %-4d\n",
			i, info->ts_nsec, info->type, info->ins_name,
			(unsigned long)info->pc,
			(unsigned long)info->phys_addr,
			(unsigned long)info->virt_addr,
			(unsigned long)info->io_data,
			info->f_start,
			info->f_end,
			info->ss_start, info->ss_end, info->core);
#else
		seq_printf(file,
			"%6d [%5lu.%06lu]   %-7s0x%-20lx0x%-13lx0x%-20lx0x%-13lx%-7d%-15pS\n",
			i++, change_format_f(info->ts_nsec),
			change_format_s(info->ts_nsec),
			((info->loginfo & LOGINFO_TYPE_MASK)
				== MMIOTRACE_TYPE_LOAD) ? "LDR" : "STR",
			(unsigned long)info->pc,
			(unsigned long)info->phys_addr,
			(unsigned long)info->virt_addr,
			(unsigned long)info->io_data,
			(info->loginfo & LOGINFO_CORE_MASK) >>	LOGINFO_CORE_SHIFT,
			(void *)info->pc);
#endif
		if (start == end)
			break;
		else
			start = (start + 1) % MMIOTRACE_LOG_BUFFER_SIZE;
	}

	return 0;
}

static int sec_mmiotrace_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, sec_mmiotrace_read, NULL);
}

static const struct file_operations mmiotrace_msg_file_ops = {
	.owner = THIS_MODULE,
	.open = sec_mmiotrace_open,
	.read = seq_read,
	.release = single_release,
	.llseek = seq_lseek,
};

static int sec_mmiotrace_probe(struct platform_device *pdev)
{
	struct device_node *node = pdev->dev.of_node, *pp;
	u32 phys_addr = 0, size = 0;
	u32 type = MMIOTRACE_TYPE_NONE;
	int ret = 0;

	if (!of_device_is_available(node))
		return ret;

	ret = of_get_child_count(node);
	if (!ret)
		return ret;

	for_each_child_of_node(node, pp) {
		const char *buf = NULL;

		of_property_read_u32(pp, "mmiotrace,addr", &phys_addr);
		of_property_read_u32(pp, "mmiotrace,size", &size);
		of_property_read_string(pp, "mmiotrace,type", &buf);

		pr_debug("%s 0x%x 0x%x %s\n",
			 __func__, phys_addr, size, buf);

		if (!strncmp(buf, "rw", 2))
			type = MMIOTRACE_TYPE_LOAD | MMIOTRACE_TYPE_STORE;
		else if (!strncmp(buf, "r", 1))
			type = MMIOTRACE_TYPE_LOAD;
		else if (!strncmp(buf, "w", 1))
			type = MMIOTRACE_TYPE_STORE;
		else
			continue;

		of_property_read_string(pp, "mmiotrace,name", &buf);

		sec_mmiotrace_commander_set((u64)phys_addr, (u64)size, type, buf);
	}

	sec_mmiotrace_trace_on_store(NULL, NULL, "1", 2);

	return 0;
}

static struct platform_driver sec_mmiotrace_driver = {
	.probe		= sec_mmiotrace_probe,
	.driver = {
		.name	= "sec_mmiotrace",
		.owner	= THIS_MODULE,
	},
};

static int __init sec_mmiotrace_late_init(void)
{
	struct proc_dir_entry *entry, *dentry;

	if (sec_debug_get_debug_level() == 0)
		return 0;

	sec_mmiotrace_init();
	platform_driver_register(&sec_mmiotrace_driver);

	dentry = proc_mkdir("mmiotrace", NULL);
	if (!dentry) {
		pr_err("%s: failed to create proc dentry\n", __func__);
		return 0;
	}

	entry = proc_create("log", S_IFREG | S_IRUGO, dentry,
			    &mmiotrace_msg_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc log\n", __func__);
		return 0;
	}
	proc_set_size(entry, 0x1000);

	entry = proc_create("addr", S_IFREG | S_IRUGO, dentry,
			    &mmiotrace_addr_file_ops);
	if (!entry) {
		pr_err("%s: failed to create proc log\n", __func__);
		return 0;
	}
	proc_set_size(entry, 0x1000);
	return 0;
}

late_initcall(sec_mmiotrace_late_init);
