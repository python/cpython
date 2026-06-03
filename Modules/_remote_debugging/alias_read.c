#include "_remote_debugging.h"

#if defined(__APPLE__) && TARGET_OS_OSX

#ifndef VM_FLAGS_RETURN_DATA_ADDR
#  define VM_FLAGS_RETURN_DATA_ADDR 0
#endif

static int
alias_direct_read(RemoteUnwinderObject *unwinder,
                  uintptr_t remote_addr,
                  size_t len,
                  void *dst)
{
    return _Py_RemoteDebug_ReadRemoteMemory(
        &unwinder->handle, remote_addr, len, dst);
}

static int
read_target_identity(RemoteUnwinderObject *unwinder,
                     uint64_t *start_tvsec,
                     uint64_t *start_tvusec)
{
    struct proc_bsdinfo info;
    int n = proc_pidinfo(unwinder->handle.pid, PROC_PIDTBSDINFO, 0,
                         &info, sizeof(info));
    if (n != (int)sizeof(info)) {
        return -1;
    }
    if (info.pbi_start_tvsec == 0 && info.pbi_start_tvusec == 0) {
        return -1;
    }
    *start_tvsec = (uint64_t)info.pbi_start_tvsec;
    *start_tvusec = (uint64_t)info.pbi_start_tvusec;
    return 0;
}

static void
alias_deallocate_entry(AliasPageEntry *entry)
{
    if (!entry->valid) {
        return;
    }
    (void)mach_vm_deallocate(mach_task_self(), entry->local_page_base,
                             entry->size);
    memset(entry, 0, sizeof(*entry));
}

void
_Py_RemoteDebug_AliasCacheClear(RemoteUnwinderObject *unwinder)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    for (int i = 0; i < MAX_ALIAS_PAGES; i++) {
        alias_deallocate_entry(&cache->pages[i]);
    }
}

static void
alias_disable_runtime(RemoteUnwinderObject *unwinder)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    cache->disabled_at_runtime = 1;
    unwinder->stats.alias_disabled_at_runtime = 1;
    _Py_RemoteDebug_AliasCacheClear(unwinder);
}

void
_Py_RemoteDebug_AliasCacheInvalidatePage(RemoteUnwinderObject *unwinder,
                                         uintptr_t remote_addr)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    size_t page_size = (size_t)unwinder->handle.page_size;
    uintptr_t page_base = remote_addr & ~(uintptr_t)(page_size - 1);

    for (int i = 0; i < MAX_ALIAS_PAGES; i++) {
        AliasPageEntry *entry = &cache->pages[i];
        if (entry->valid
                && entry->remote_page_base == page_base
                && entry->task_port == unwinder->handle.task
                && entry->target_start_tvsec == cache->target_start_tvsec
                && entry->target_start_tvusec == cache->target_start_tvusec) {
            alias_deallocate_entry(entry);
        }
    }
}

void
_Py_RemoteDebug_AliasCacheInit(RemoteUnwinderObject *unwinder)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    memset(cache, 0, sizeof(*cache));

    uint64_t start_tvsec = 0;
    uint64_t start_tvusec = 0;
    uint64_t reprobe_tvsec = 0;
    uint64_t reprobe_tvusec = 0;
    if (read_target_identity(unwinder, &start_tvsec, &start_tvusec) < 0
            || read_target_identity(unwinder, &reprobe_tvsec,
                                    &reprobe_tvusec) < 0
            || start_tvsec != reprobe_tvsec
            || start_tvusec != reprobe_tvusec) {
        cache->disabled_at_init = 1;
    }
    else {
        cache->target_start_tvsec = start_tvsec;
        cache->target_start_tvusec = start_tvusec;
    }
    unwinder->stats.alias_disabled_at_init =
        (uint64_t)cache->disabled_at_init;
}

static int
alias_identity_matches(RemoteUnwinderObject *unwinder)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    uint64_t start_tvsec = 0;
    uint64_t start_tvusec = 0;
    if (read_target_identity(unwinder, &start_tvsec, &start_tvusec) < 0) {
        return 0;
    }
    return start_tvsec == cache->target_start_tvsec
        && start_tvusec == cache->target_start_tvusec;
}

static int
alias_maybe_probe_identity(RemoteUnwinderObject *unwinder)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    if ((++cache->probe_counter & ALIAS_PROBE_MASK) != 0) {
        return 1;
    }
    if (alias_identity_matches(unwinder)) {
        return 1;
    }
    STATS_INC(unwinder, alias_identity_mismatches);
    alias_disable_runtime(unwinder);
    return 0;
}

static void
alias_record_remap_outcome(RemoteUnwinderObject *unwinder, int failed)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    unsigned char old = cache->remap_failure_window[cache->remap_failure_index];
    if (old) {
        cache->remap_failure_count--;
    }
    cache->remap_failure_window[cache->remap_failure_index] =
        (unsigned char)(failed != 0);
    if (failed) {
        cache->remap_failure_count++;
    }
    cache->remap_failure_index =
        (cache->remap_failure_index + 1) % ALIAS_FAILURE_WINDOW;
    if (cache->remap_failure_samples < ALIAS_FAILURE_WINDOW) {
        cache->remap_failure_samples++;
    }

    if (cache->remap_failure_samples == ALIAS_FAILURE_WINDOW
            && cache->remap_failure_count >= ALIAS_FAILURE_THRESHOLD) {
        alias_disable_runtime(unwinder);
    }
}

static AliasPageEntry *
alias_find_entry(RemoteUnwinderObject *unwinder, uintptr_t page_base)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    for (int i = 0; i < MAX_ALIAS_PAGES; i++) {
        AliasPageEntry *entry = &cache->pages[i];
        if (entry->valid
                && entry->remote_page_base == page_base
                && entry->task_port == unwinder->handle.task
                && entry->target_start_tvsec == cache->target_start_tvsec
                && entry->target_start_tvusec == cache->target_start_tvusec) {
            return entry;
        }
    }
    return NULL;
}

static AliasPageEntry *
alias_alloc_entry(RemoteUnwinderObject *unwinder)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    AliasPageEntry *oldest = NULL;

    for (int i = 0; i < MAX_ALIAS_PAGES; i++) {
        AliasPageEntry *entry = &cache->pages[i];
        if (!entry->valid) {
            return entry;
        }
        if (oldest == NULL || entry->access_seq < oldest->access_seq) {
            oldest = entry;
        }
    }

    assert(oldest != NULL);
    alias_deallocate_entry(oldest);
    STATS_INC(unwinder, alias_evictions);
    return oldest;
}

static int
alias_remap_page(RemoteUnwinderObject *unwinder,
                 uintptr_t page_base,
                 AliasPageEntry **entry_out)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    mach_vm_size_t page_size = (mach_vm_size_t)unwinder->handle.page_size;
    mach_vm_address_t local_addr = 0;
    vm_prot_t cur_protection = VM_PROT_NONE;
    vm_prot_t max_protection = VM_PROT_NONE;

    kern_return_t kr = mach_vm_remap(
        mach_task_self(),
        &local_addr,
        page_size,
        0,
        VM_FLAGS_ANYWHERE | VM_FLAGS_RETURN_DATA_ADDR,
        unwinder->handle.task,
        (mach_vm_address_t)page_base,
        FALSE,
        &cur_protection,
        &max_protection,
        VM_INHERIT_NONE);
    if (kr != KERN_SUCCESS) {
        STATS_INC(unwinder, alias_remap_failures);
        alias_record_remap_outcome(unwinder, 1);
        return -1;
    }
    if ((cur_protection & VM_PROT_READ) == 0) {
        (void)mach_vm_deallocate(mach_task_self(), local_addr, page_size);
        STATS_INC(unwinder, alias_remap_failures);
        alias_record_remap_outcome(unwinder, 1);
        return -1;
    }

    kr = mach_vm_protect(mach_task_self(), local_addr, page_size, FALSE,
                         VM_PROT_READ);
    if (kr != KERN_SUCCESS) {
        (void)mach_vm_deallocate(mach_task_self(), local_addr, page_size);
        STATS_INC(unwinder, alias_remap_failures);
        alias_record_remap_outcome(unwinder, 1);
        return -1;
    }

    AliasPageEntry *entry = alias_alloc_entry(unwinder);
    memset(entry, 0, sizeof(*entry));
    entry->remote_page_base = page_base;
    entry->local_page_base = local_addr;
    entry->size = page_size;
    entry->task_port = unwinder->handle.task;
    entry->target_start_tvsec = cache->target_start_tvsec;
    entry->target_start_tvusec = cache->target_start_tvusec;
    entry->access_seq = ++cache->access_seq;
    entry->valid = 1;

    alias_record_remap_outcome(unwinder, 0);
    *entry_out = entry;
    return 0;
}

int
_Py_RemoteDebug_AliasProbe(RemoteUnwinderObject *unwinder,
                           uintptr_t probe_addr)
{
    AliasReadCache *cache = &unwinder->alias_cache;
    if (cache->disabled_at_init || cache->disabled_at_runtime) {
        return -1;
    }

    size_t page_size = (size_t)unwinder->handle.page_size;
    uintptr_t page_base = probe_addr & ~(uintptr_t)(page_size - 1);
    mach_vm_address_t local_addr = 0;
    vm_prot_t cur_protection = VM_PROT_NONE;
    vm_prot_t max_protection = VM_PROT_NONE;
    kern_return_t kr = mach_vm_remap(
        mach_task_self(),
        &local_addr,
        (mach_vm_size_t)page_size,
        0,
        VM_FLAGS_ANYWHERE | VM_FLAGS_RETURN_DATA_ADDR,
        unwinder->handle.task,
        (mach_vm_address_t)page_base,
        FALSE,
        &cur_protection,
        &max_protection,
        VM_INHERIT_NONE);
    if (kr != KERN_SUCCESS || (cur_protection & VM_PROT_READ) == 0) {
        if (kr == KERN_SUCCESS) {
            (void)mach_vm_deallocate(mach_task_self(), local_addr,
                                     (mach_vm_size_t)page_size);
        }
        STATS_INC(unwinder, alias_remap_failures);
        alias_record_remap_outcome(unwinder, 1);
        return -1;
    }

    kr = mach_vm_protect(mach_task_self(), local_addr,
                         (mach_vm_size_t)page_size, FALSE, VM_PROT_READ);
    (void)mach_vm_deallocate(mach_task_self(), local_addr,
                             (mach_vm_size_t)page_size);
    if (kr != KERN_SUCCESS) {
        STATS_INC(unwinder, alias_remap_failures);
        alias_record_remap_outcome(unwinder, 1);
        return -1;
    }

    alias_record_remap_outcome(unwinder, 0);
    return 0;
}

int
_Py_RemoteDebug_AliasedRead(RemoteUnwinderObject *unwinder,
                            AliasReadKind kind,
                            uintptr_t remote_addr,
                            size_t len,
                            void *dst)
{
    (void)kind;
    AliasReadCache *cache = &unwinder->alias_cache;
    if (len == 0) {
        return 0;
    }
    if (cache->disabled_at_init || cache->disabled_at_runtime) {
        return alias_direct_read(unwinder, remote_addr, len, dst);
    }

    size_t page_size = (size_t)unwinder->handle.page_size;
    uintptr_t page_base = remote_addr & ~(uintptr_t)(page_size - 1);
    size_t offset = (size_t)(remote_addr - page_base);
    if (offset >= page_size || len > page_size - offset) {
        return alias_direct_read(unwinder, remote_addr, len, dst);
    }

    AliasPageEntry *entry = alias_find_entry(unwinder, page_base);
    if (entry != NULL) {
        if (!alias_maybe_probe_identity(unwinder)) {
            return alias_direct_read(unwinder, remote_addr, len, dst);
        }
        entry->access_seq = ++cache->access_seq;
        memcpy(dst, (const char *)entry->local_page_base + offset, len);
        STATS_INC(unwinder, alias_hits);
        return 0;
    }

    STATS_INC(unwinder, alias_misses);
    if (alias_remap_page(unwinder, page_base, &entry) < 0) {
        return alias_direct_read(unwinder, remote_addr, len, dst);
    }
    if (cache->disabled_at_runtime) {
        return alias_direct_read(unwinder, remote_addr, len, dst);
    }
    memcpy(dst, (const char *)entry->local_page_base + offset, len);
    return 0;
}

#endif /* defined(__APPLE__) && TARGET_OS_OSX */
