#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/kernfs.h>
#include <linux/mutex.h>
#include <linux/kallsyms.h>
#include <linux/seq_file.h>
#include <linux/kobject.h>


static inline struct file_system_type *get_sysfs_system_type(void)
{
	struct file_system_type *sysfs_type = get_fs_type("sysfs");

	if (!sysfs_type) {
		pr_err("[%s] sysfs isn't registered\n", __func__);
		return NULL;
	}

	return sysfs_type;
}


static inline void put_sysfs_system_type(struct file_system_type *sysfs_type)
{
	module_put(sysfs_type->owner);
}


static struct kernfs_node *__sysfs_root_kn = NULL;
static struct mutex *__kernfs_mutex = NULL;
static struct kset *__module_kset;

static inline bool iter_kobject_has_children(struct kobject *kobj)
{
        WARN_ON_ONCE(kref_read(&kobj->kref) == 0);

        return kobj->sd && kobj->sd->dir.subdirs;
}

struct iterate_sysfs_files_t {
	bool (*filter_call)(struct kernfs_node *kn);
	int  (*callback_call)(void *data);
};


static bool filter_wrapper(struct kernfs_node *kn)
{
	return true;
}

static int print_kernfs(void *data) 
{
	struct { struct kernfs_node *kn; int tabs; } *print_ctx = data;
	pr_err(	"[%s] %*s kn[%lx] kn_name[%s]\n",
		__func__, print_ctx->tabs, "\t", print_ctx->kn, print_ctx->kn->name);
	return 0;
}

#define rb_to_kn(X) rb_entry((X), struct kernfs_node, rb)
static void iterate_subdir(struct kernfs_node *kn,
			int tabs, const struct iterate_sysfs_files_t *ctx)
{
	struct rb_node *node = kn->dir.children.rb_node;

	if (IS_ERR_OR_NULL(ctx)) {
		pr_err("[%s] there is no context here\n", __func__);
		return -EINVAL;
	}

	if (IS_ERR_OR_NULL(ctx->filter_call) || IS_ERR_OR_NULL(ctx->callback_call)) {
		pr_err("[%s] there are no callbacks here\n", __func__);
		return -EINVAL;
	}

	while (node) {
		struct kernfs_node *pos = rb_to_kn(node);

		kernfs_get(pos);
		if (ctx->filter_call(pos)) {
			struct { struct kernfs_node *kn; int tabs; } data = {pos, tabs};
			ctx->callback_call((void*)&data);
		}

		if (kernfs_type(pos) == KERNFS_DIR)
			iterate_subdir(pos, tabs+1, ctx);
		kernfs_put(pos);
		node = rb_next(node);
	}
}

static int kset_iterate_objs(struct kset *kset)
{
        struct kobject *k, *parent = NULL;
	const struct iterate_sysfs_files_t ctx = {
		.filter_call = filter_wrapper,
		.callback_call = print_kernfs 
	};


	kset_get(kset);
        spin_lock(&kset->list_lock);

        list_for_each_entry(k, &kset->list, entry) {
		if (!k->state_initialized) {
			pr_err("[%s] kobj[%lx] wasn't initialized\n", __func__, k);
			continue;
		}

		kobject_get(k);
		if (k->name && k->parent && !parent) {
			parent = k->parent;
			kobject_put(k);
			break;
		}
		kobject_put(k);
        }

        spin_unlock(&kset->list_lock);
	kset_put(kset);

	if (parent) {
		kobject_get(parent);
		mutex_lock(__kernfs_mutex);
		kernfs_get(parent->sd);
	
		iterate_subdir(parent->sd, 1, &ctx);

		kernfs_put(parent->sd);
		mutex_unlock(__kernfs_mutex);
		kobject_put(parent);
	}
        return 0;
}

static int walk_sysfs_kernfs_nodes(void)
{
	struct file_system_type *sysfs_type;

	sysfs_type = get_sysfs_system_type();
	if (!sysfs_type) {
		return -ESRCH;
	}

	if (!__sysfs_root_kn) {
		pr_err("[%s] there is no sysfs_root_kn\n", __func__);
		return -EINVAL;
	}

	kset_iterate_objs(__module_kset);

	put_sysfs_system_type(sysfs_type);
	pr_err("[%s] end\n", __func__);
	return 0;
}

static int __init iterate_sysfs_files_init(void)
{
	__sysfs_root_kn = (struct kernfs_node *)kallsyms_lookup_name("sysfs_root_kn");
	if (!__sysfs_root_kn) {
		pr_err("[%s] kallsyms couldn't find sysfs_root_kn\n", __func__);
		return -ESRCH;
	}

	__kernfs_mutex = (struct mutex *)kallsyms_lookup_name("kernfs_mutex");
	if (!__kernfs_mutex) {
		pr_err("[%s] kallsyms couldn't find kernfs_mutex\n", __func__);
		return -ESRCH;
	}

	__module_kset = (struct kset *)kallsyms_lookup_name("module_kset");
	if (!__module_kset) {
		pr_err("[%s] kallsyms couldn't find module_kset\n", __func__);
		return -ESRCH;
	}

	walk_sysfs_kernfs_nodes();
	return 0;
}

static void __exit iterate_sysfs_files_exit(void)
{
	pr_info("[%s] exit\n", __func__);
}

module_init(iterate_sysfs_files_init);
module_exit(iterate_sysfs_files_exit);

MODULE_LICENSE("GPL");
