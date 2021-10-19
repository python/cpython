/* ----------------------------------------------------------------------------
Copyright (c) 2018-2020, Microsoft Research, Daan Leijen
This is free software; you can redistribute it and/or modify it under the
terms of the MIT license. A copy of the license can be found in the file
"LICENSE" at the root of this distribution.
-----------------------------------------------------------------------------*/

#include "mimalloc.h"
#include "mimalloc-internal.h"

#if defined(MI_MALLOC_OVERRIDE)

#if !defined(__APPLE__)
#error "this file should only be included on macOS"
#endif

/* ------------------------------------------------------
   Override system malloc on macOS
   This is done through the malloc zone interface.
   It seems we also need to interpose (see `alloc-override.c`)
   or otherwise we get zone errors as there are usually 
   already allocations done by the time we take over the 
   zone. Unfortunately, that means we need to replace
   the `free` with a checked free (`cfree`) impacting 
   performance.
------------------------------------------------------ */

#include <AvailabilityMacros.h>
#include <malloc/malloc.h>
#include <string.h>  // memset

#if defined(MAC_OS_X_VERSION_10_6) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
// only available from OSX 10.6
extern malloc_zone_t* malloc_default_purgeable_zone(void) __attribute__((weak_import));
#endif

/* ------------------------------------------------------
   malloc zone members
------------------------------------------------------ */

static size_t zone_size(malloc_zone_t* zone, const void* p) {
  UNUSED(zone);
  if (!mi_is_in_heap_region(p))
    return 0; // not our pointer, bail out

  return mi_usable_size(p);
}

static void* zone_malloc(malloc_zone_t* zone, size_t size) {
  UNUSED(zone);
  return mi_malloc(size);
}

static void* zone_calloc(malloc_zone_t* zone, size_t count, size_t size) {
  UNUSED(zone);
  return mi_calloc(count, size);
}

static void* zone_valloc(malloc_zone_t* zone, size_t size) {
  UNUSED(zone);
  return mi_malloc_aligned(size, _mi_os_page_size());
}

static void zone_free(malloc_zone_t* zone, void* p) {
  UNUSED(zone);
  mi_free(p);
}

static void* zone_realloc(malloc_zone_t* zone, void* p, size_t newsize) {
  UNUSED(zone);
  return mi_realloc(p, newsize);
}

static void* zone_memalign(malloc_zone_t* zone, size_t alignment, size_t size) {
  UNUSED(zone);
  return mi_malloc_aligned(size,alignment);
}

static void zone_destroy(malloc_zone_t* zone) {
  UNUSED(zone);
  // todo: ignore for now?
}

static unsigned zone_batch_malloc(malloc_zone_t* zone, size_t size, void** ps, unsigned count) {
  size_t i;
  for (i = 0; i < count; i++) {
    ps[i] = zone_malloc(zone, size);
    if (ps[i] == NULL) break;
  }
  return i;
}

static void zone_batch_free(malloc_zone_t* zone, void** ps, unsigned count) {
  for(size_t i = 0; i < count; i++) {
    zone_free(zone, ps[i]);
    ps[i] = NULL;
  }
}

static size_t zone_pressure_relief(malloc_zone_t* zone, size_t size) {
  UNUSED(zone); UNUSED(size);
  mi_collect(false);
  return 0;
}

static void zone_free_definite_size(malloc_zone_t* zone, void* p, size_t size) {
  UNUSED(size);
  zone_free(zone,p);
}


/* ------------------------------------------------------
   Introspection members
------------------------------------------------------ */

static kern_return_t intro_enumerator(task_t task, void* p,
                            unsigned type_mask, vm_address_t zone_address,
                            memory_reader_t reader,
                            vm_range_recorder_t recorder)
{
  // todo: enumerate all memory
  UNUSED(task); UNUSED(p); UNUSED(type_mask); UNUSED(zone_address);
  UNUSED(reader); UNUSED(recorder);
  return KERN_SUCCESS;
}

static size_t intro_good_size(malloc_zone_t* zone, size_t size) {
  UNUSED(zone);
  return mi_good_size(size);
}

static boolean_t intro_check(malloc_zone_t* zone) {
  UNUSED(zone);
  return true;
}

static void intro_print(malloc_zone_t* zone, boolean_t verbose) {
  UNUSED(zone); UNUSED(verbose);
  mi_stats_print(NULL);
}

static void intro_log(malloc_zone_t* zone, void* p) {
  UNUSED(zone); UNUSED(p);
  // todo?
}

static void intro_force_lock(malloc_zone_t* zone) {
  UNUSED(zone);
  // todo?
}

static void intro_force_unlock(malloc_zone_t* zone) {
  UNUSED(zone);
  // todo?
}

static void intro_statistics(malloc_zone_t* zone, malloc_statistics_t* stats) {
  UNUSED(zone);
  // todo...
  stats->blocks_in_use = 0;
  stats->size_in_use = 0;
  stats->max_size_in_use = 0;
  stats->size_allocated = 0;
}

static boolean_t intro_zone_locked(malloc_zone_t* zone) {
  UNUSED(zone);
  return false;
}


/* ------------------------------------------------------
  At process start, override the default allocator
------------------------------------------------------ */

static malloc_zone_t* mi_get_default_zone()
{
  // The first returned zone is the real default
  malloc_zone_t** zones = NULL;
  unsigned count = 0;
  kern_return_t ret = malloc_get_all_zones(0, NULL, (vm_address_t**)&zones, &count);
  if (ret == KERN_SUCCESS && count > 0) {
    return zones[0];
  }
  else {
    // fallback
    return malloc_default_zone();
  }
}

static malloc_introspection_t mi_introspect = {
  .enumerator = &intro_enumerator,
  .good_size = &intro_good_size,
  .check = &intro_check,
  .print = &intro_print,
  .log = &intro_log,
  .force_lock = &intro_force_lock,
  .force_unlock = &intro_force_unlock,
#if defined(MAC_OS_X_VERSION_10_6) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
  .zone_locked = &intro_zone_locked,
  .statistics = &intro_statistics,
#endif
};

static malloc_zone_t mi_malloc_zone = {
  .size = &zone_size,
  .zone_name = "mimalloc",
  .introspect = &mi_introspect,
  .malloc = &zone_malloc,
  .calloc = &zone_calloc,
  .valloc = &zone_valloc,
  .free = &zone_free,
  .realloc = &zone_realloc,
  .destroy = &zone_destroy,
  .batch_malloc = &zone_batch_malloc,
  .batch_free = &zone_batch_free,
#if defined(MAC_OS_X_VERSION_10_6) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
  // switch to version 9 on OSX 10.6 to support memalign.
  .version = 9,
  .memalign = &zone_memalign,
  .free_definite_size = &zone_free_definite_size,
  .pressure_relief = &zone_pressure_relief,
#else
  .version = 4,
#endif
};


#if defined(MI_SHARED_LIB_EXPORT) && defined(MI_INTERPOSE)

static malloc_zone_t *mi_malloc_default_zone(void) {
  return &mi_malloc_zone;
}
// TODO: should use the macros in alloc-override but they aren't available here.
__attribute__((used)) static struct {
  const void *replacement;
  const void *target;
} replace_malloc_default_zone[] __attribute__((section("__DATA, __interpose"))) = {
  { (const void*)mi_malloc_default_zone, (const void*)malloc_default_zone },
};
#endif

static void __attribute__((constructor(0))) _mi_macos_override_malloc() {
  malloc_zone_t* purgeable_zone = NULL;

#if defined(MAC_OS_X_VERSION_10_6) && \
    MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_6
  // force the purgeable zone to exist to avoid strange bugs
  if (malloc_default_purgeable_zone) {
    purgeable_zone = malloc_default_purgeable_zone();
  }
#endif

  // Register our zone.
  // thomcc: I think this is still needed to put us in the zone list.
  malloc_zone_register(&mi_malloc_zone);
  // Unregister the default zone, this makes our zone the new default
  // as that was the last registered.
  malloc_zone_t *default_zone = mi_get_default_zone();
  // thomcc: Unsure if the next test is *always* false or just false in the
  // cases I've tried. I'm also unsure if the code inside is needed. at all
  if (default_zone != &mi_malloc_zone) {
    malloc_zone_unregister(default_zone);

    // Reregister the default zone so free and realloc in that zone keep working.
    malloc_zone_register(default_zone);
  }

  // Unregister, and re-register the purgeable_zone to avoid bugs if it occurs
  // earlier than the default zone.
  if (purgeable_zone != NULL) {
    malloc_zone_unregister(purgeable_zone);
    malloc_zone_register(purgeable_zone);
  }

}

#endif // MI_MALLOC_OVERRIDE
