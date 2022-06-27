# Iterate sysfs kernel descriptors
## Collections of kobjects

First of all, this module uses ksets: collection of kobjects. Which associated with sysfs folders (for example, module_kset for /sys/modules)
include/linux/kobject.h
```c
/**
 * struct kset - a set of kobjects of a specific type, belonging to a specific subsystem.
 *
 * A kset defines a group of kobjects.  They can be individually
 * different "types" but overall these kobjects all want to be grouped
 * together and operated on in the same manner.  ksets are used to
 * define the attribute callbacks and other common events that happen to
 * a kobject.
 *
 * @list: the list of all kobjects for this kset
 * @list_lock: a lock for iterating over the kobjects
 * @kobj: the embedded kobject for this kset (recursion, isn't it fun...)
 * @uevent_ops: the set of uevent operations for this kset.  These are
 * called whenever a kobject has something happen to it so that the kset
 * can add new environment variables, or filter out the uevents if so
 * desired.
 */
struct kset {
     struct list_head list; <======== walk list
     spinlock_t list_lock;
     struct kobject kobj;
     const struct kset_uevent_ops *uevent_ops;
} __randomize_layout;
```
kset is a group of kobject's that is the view in sysfs/ reference counter, hotplug handler and exc.
```c
struct kobject {
     const char              *name;
     struct list_head        entry;
     struct kobject          *parent;
     struct kset             *kset;
     struct kobj_type        *ktype;
     struct kernfs_node      *sd; /* sysfs directory entry */ <====== use to walk subdirs
     struct kref             kref;
#ifdef CONFIG_DEBUG_KOBJECT_RELEASE
     struct delayed_work     release;
#endif
     unsigned int state_initialized:1;
     unsigned int state_in_sysfs:1;
     unsigned int state_add_uevent_sent:1;
     unsigned int state_remove_uevent_sent:1;
     unsigned int uevent_suppress:1;
};
```
kernfs_node (kn->dir.children.rb_node) can be used to walk through subdirs if it is KERNFS_DIR:
```c
/*
 * kernfs_node - the building block of kernfs hierarchy.  Each and every
 * kernfs node is represented by single kernfs_node.  Most fields are
 * private to kernfs and shouldn't be accessed directly by kernfs users.
 *
 * As long as s_count reference is held, the kernfs_node itself is
 * accessible.  Dereferencing elem or any other outer entity requires
 * active reference.
 */
struct kernfs_node {
     atomic_t                count;
     atomic_t                active;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
     struct lockdep_map      dep_map;
#endif
     /*
      * Use kernfs_get_parent() and kernfs_name/path() instead of
      * accessing the following two fields directly.  If the node is
      * never moved to a different parent, it is safe to access the
      * parent directly.
      */
     struct kernfs_node      *parent;
     const char              *name;

     struct rb_node          rb;

     const void              *ns;    /* namespace tag */
     unsigned int            hash;   /* ns + name hash */
     union {
             struct kernfs_elem_dir          dir;
             struct kernfs_elem_symlink      symlink;
             struct kernfs_elem_attr         attr;
     };

     void                    *priv;

     union kernfs_node_id    id;
     unsigned short          flags;
     umode_t                 mode;
     struct kernfs_iattrs    *iattr;
};
```

## Filters and callbacks

Use filters and callbacks to handle kernfs representation of sysfs file. In this example all files from /sys/modules are iterated and printed their kernfs_node addresses and names.

## Build

1. Install prerequisites for debian based distros:
``sudo apt install build-essential``
for redhat based disros:
``sudo dnf group install "Development Tools"``
2. Install kernel-headers for debian based distros:
``sudo apt install linux-headers-$(uname -r)``
for redhat based distros:
``sudo dnf instal kernel-devel-$(uname -r)``
3. build lkm:
``make clean all``

## Test

Test it!
``insmod iterate_sysfs_files.ko``



