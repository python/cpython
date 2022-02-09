#ifndef Py_INTERNAL_MIMALLOC_H
#define Py_INTERNAL_MIMALLOC_H

#ifndef Py_BUILD_CORE
#  error "this header requires Py_BUILD_CORE define"
#endif

#if defined(MIMALLOC_H) || defined(MIMALLOC_TYPES_H)
#  error "pycore_mimalloc.h must be included before mimalloc.h"
#endif

#include "pycore_pymem.h"
#define MI_DEBUG_UNINIT     PYMEM_CLEANBYTE
#define MI_DEBUG_FREED      PYMEM_DEADBYTE
#define MI_DEBUG_PADDING    PYMEM_FORBIDDENBYTE

 /* ASAN builds don't use MI_DEBUG.
  *
  * ASAN + MI_DEBUG triggers additional checks, which can cause mimalloc
  * to print warnings to stderr. The warnings break some tests.
  *
  *   mi_usable_size: pointer might not point to a valid heap region:
  *   ...
  *   yes, the previous pointer ... was valid after all
  */
#if defined(__has_feature)
#  if __has_feature(address_sanitizer)
#    define MI_DEBUG 0
#  endif
#elif defined(__GNUC__) && defined(__SANITIZE_ADDRESS__)
#  define MI_DEBUG 0
#endif

#ifdef Py_DEBUG
/* Debug: Perform additional checks in debug builds, see mimalloc-types.h
 * - enable basic and internal assertion checks with MI_DEBUG 2
 * - check for double free, invalid pointer free
 * - use guard pages to check for buffer overflows
 */
#  ifndef MI_DEBUG
#    define MI_DEBUG 2
#  endif
#  define MI_SECURE 4
#else
// Production: no debug checks, secure depends on --enable-mimalloc-secure
#  ifndef MI_DEBUG
#    define MI_DEBUG 0
#  endif
#  if defined(PY_MIMALLOC_SECURE)
#    define MI_SECURE PY_MIMALLOC_SECURE
#  else
#    define MI_SECURE 0
#  endif
#endif

/* Prefix all non-static symbols with "_Py_"
 * nm Objects/obmalloc.o | grep -E "[CRT] _?mi" | awk '{print "#define " $3 " _Py_" $3}' | sort
 */
#if 1
#define _mi_abandoned_await_readers _Py__mi_abandoned_await_readers
#define _mi_abandoned_reclaim_all _Py__mi_abandoned_reclaim_all
#define mi_aligned_alloc _Py_mi_aligned_alloc
#define mi_aligned_offset_recalloc _Py_mi_aligned_offset_recalloc
#define mi_aligned_recalloc _Py_mi_aligned_recalloc
#define _mi_arena_alloc_aligned _Py__mi_arena_alloc_aligned
#define _mi_arena_alloc _Py__mi_arena_alloc
#define _mi_arena_free _Py__mi_arena_free
#define _mi_bin _Py__mi_bin
#define _mi_bin_size _Py__mi_bin_size
#define _mi_bitmap_claim_across _Py__mi_bitmap_claim_across
#define _mi_bitmap_claim _Py__mi_bitmap_claim
#define _mi_bitmap_is_any_claimed_across _Py__mi_bitmap_is_any_claimed_across
#define _mi_bitmap_is_any_claimed _Py__mi_bitmap_is_any_claimed
#define _mi_bitmap_is_claimed_across _Py__mi_bitmap_is_claimed_across
#define _mi_bitmap_is_claimed _Py__mi_bitmap_is_claimed
#define _mi_bitmap_try_find_claim_field _Py__mi_bitmap_try_find_claim_field
#define _mi_bitmap_try_find_from_claim_across _Py__mi_bitmap_try_find_from_claim_across
#define _mi_bitmap_try_find_from_claim _Py__mi_bitmap_try_find_from_claim
#define _mi_bitmap_unclaim_across _Py__mi_bitmap_unclaim_across
#define _mi_bitmap_unclaim _Py__mi_bitmap_unclaim
#define _mi_block_zero_init _Py__mi_block_zero_init
#define mi_calloc_aligned_at _Py_mi_calloc_aligned_at
#define mi_calloc_aligned _Py_mi_calloc_aligned
#define mi_calloc _Py_mi_calloc
#define mi_cfree _Py_mi_cfree
#define mi_check_owned _Py_mi_check_owned
#define _mi_clock_end _Py__mi_clock_end
#define _mi_clock_now _Py__mi_clock_now
#define _mi_clock_start _Py__mi_clock_start
#define mi_collect _Py_mi_collect
#define _mi_commit_mask_committed_size _Py__mi_commit_mask_committed_size
#define _mi_commit_mask_next_run _Py__mi_commit_mask_next_run
#define _mi_current_thread_count _Py__mi_current_thread_count
#define mi_debug_show_arenas _Py_mi_debug_show_arenas
#define _mi_deferred_free _Py__mi_deferred_free
#define mi_dupenv_s _Py_mi_dupenv_s
#define _mi_error_message _Py__mi_error_message
#define mi__expand _Py_mi__expand
#define mi_expand _Py_mi_expand
#define _mi_fprintf _Py__mi_fprintf
#define _mi_fputs _Py__mi_fputs
#define mi_free_aligned _Py_mi_free_aligned
#define _mi_free_delayed_block _Py__mi_free_delayed_block
#define mi_free _Py_mi_free
#define mi_free_size_aligned _Py_mi_free_size_aligned
#define mi_free_size _Py_mi_free_size
#define mi_good_size _Py_mi_good_size
#define mi_heap_calloc_aligned_at _Py_mi_heap_calloc_aligned_at
#define mi_heap_calloc_aligned _Py_mi_heap_calloc_aligned
#define mi_heap_calloc _Py_mi_heap_calloc
#define mi_heap_check_owned _Py_mi_heap_check_owned
#define _mi_heap_collect_abandon _Py__mi_heap_collect_abandon
#define mi_heap_collect _Py_mi_heap_collect
#define _mi_heap_collect_retired _Py__mi_heap_collect_retired
#define mi_heap_contains_block _Py_mi_heap_contains_block
#define _mi_heap_delayed_free _Py__mi_heap_delayed_free
#define mi_heap_delete _Py_mi_heap_delete
#define _mi_heap_destroy_pages _Py__mi_heap_destroy_pages
#define mi_heap_destroy _Py_mi_heap_destroy
#define _mi_heap_empty _Py__mi_heap_empty
#define mi_heap_get_backing _Py_mi_heap_get_backing
#define mi_heap_get_default _Py_mi_heap_get_default
#define _mi_heap_main_get _Py__mi_heap_main_get
#define mi_heap_malloc_aligned_at _Py_mi_heap_malloc_aligned_at
#define mi_heap_malloc_aligned _Py_mi_heap_malloc_aligned
#define mi_heap_mallocn _Py_mi_heap_mallocn
#define mi_heap_malloc _Py_mi_heap_malloc
#define mi_heap_malloc_small _Py_mi_heap_malloc_small
#define _mi_heap_malloc_zero _Py__mi_heap_malloc_zero
#define mi_heap_new _Py_mi_heap_new
#define _mi_heap_random_next _Py__mi_heap_random_next
#define mi_heap_realloc_aligned_at _Py_mi_heap_realloc_aligned_at
#define mi_heap_realloc_aligned _Py_mi_heap_realloc_aligned
#define mi_heap_reallocf _Py_mi_heap_reallocf
#define mi_heap_reallocn _Py_mi_heap_reallocn
#define mi_heap_realloc _Py_mi_heap_realloc
#define _mi_heap_realloc_zero _Py__mi_heap_realloc_zero
#define mi_heap_realpath _Py_mi_heap_realpath
#define mi_heap_recalloc_aligned_at _Py_mi_heap_recalloc_aligned_at
#define mi_heap_recalloc_aligned _Py_mi_heap_recalloc_aligned
#define mi_heap_recalloc _Py_mi_heap_recalloc
#define mi_heap_rezalloc_aligned_at _Py_mi_heap_rezalloc_aligned_at
#define mi_heap_rezalloc_aligned _Py_mi_heap_rezalloc_aligned
#define mi_heap_rezalloc _Py_mi_heap_rezalloc
#define _mi_heap_set_default_direct _Py__mi_heap_set_default_direct
#define mi_heap_set_default _Py_mi_heap_set_default
#define mi_heap_strdup _Py_mi_heap_strdup
#define mi_heap_strndup _Py_mi_heap_strndup
#define mi_heap_visit_blocks _Py_mi_heap_visit_blocks
#define mi_heap_zalloc_aligned_at _Py_mi_heap_zalloc_aligned_at
#define mi_heap_zalloc_aligned _Py_mi_heap_zalloc_aligned
#define mi_heap_zalloc _Py_mi_heap_zalloc
#define mi_is_in_heap_region _Py_mi_is_in_heap_region
#define _mi_is_main_thread _Py__mi_is_main_thread
#define mi_is_redirected _Py_mi_is_redirected
#define mi_malloc_aligned_at _Py_mi_malloc_aligned_at
#define mi_malloc_aligned _Py_mi_malloc_aligned
#define _mi_malloc_generic _Py__mi_malloc_generic
#define mi_malloc_good_size _Py_mi_malloc_good_size
#define mi_mallocn _Py_mi_mallocn
#define mi_malloc _Py_mi_malloc
#define mi_malloc_size _Py_mi_malloc_size
#define mi_malloc_small _Py_mi_malloc_small
#define mi_malloc_usable_size _Py_mi_malloc_usable_size
#define mi_manage_os_memory _Py_mi_manage_os_memory
#define mi_mbsdup _Py_mi_mbsdup
#define mi_memalign _Py_mi_memalign
#define mi_new_aligned_nothrow _Py_mi_new_aligned_nothrow
#define mi_new_aligned _Py_mi_new_aligned
#define mi_new_nothrow _Py_mi_new_nothrow
#define mi_new_n _Py_mi_new_n
#define mi_new _Py_mi_new
#define mi_new_reallocn _Py_mi_new_reallocn
#define mi_new_realloc _Py_mi_new_realloc
#define mi_option_disable _Py_mi_option_disable
#define mi_option_enable _Py_mi_option_enable
#define mi_option_get _Py_mi_option_get
#define mi_option_is_enabled _Py_mi_option_is_enabled
#define mi_option_set_default _Py_mi_option_set_default
#define mi_option_set_enabled_default _Py_mi_option_set_enabled_default
#define mi_option_set_enabled _Py_mi_option_set_enabled
#define mi_option_set _Py_mi_option_set
#define _mi_options_init _Py__mi_options_init
#define _mi_os_alloc_aligned _Py__mi_os_alloc_aligned
#define _mi_os_alloc_huge_os_pages _Py__mi_os_alloc_huge_os_pages
#define _mi_os_alloc _Py__mi_os_alloc
#define _mi_os_commit _Py__mi_os_commit
#define _mi_os_decommit _Py__mi_os_decommit
#define _mi_os_free_ex _Py__mi_os_free_ex
#define _mi_os_free_huge_pages _Py__mi_os_free_huge_pages
#define _mi_os_free _Py__mi_os_free
#define _mi_os_good_alloc_size _Py__mi_os_good_alloc_size
#define _mi_os_has_overcommit _Py__mi_os_has_overcommit
#define _mi_os_init _Py__mi_os_init
#define _mi_os_large_page_size _Py__mi_os_large_page_size
#define _mi_os_numa_node_count_get _Py__mi_os_numa_node_count_get
#define _mi_os_numa_node_get _Py__mi_os_numa_node_get
#define _mi_os_page_size _Py__mi_os_page_size
#define _mi_os_protect _Py__mi_os_protect
#define _mi_os_random_weak _Py__mi_os_random_weak
#define _mi_os_reset _Py__mi_os_reset
#define _mi_os_shrink _Py__mi_os_shrink
#define _mi_os_unprotect _Py__mi_os_unprotect
#define _mi_os_unreset _Py__mi_os_unreset
#define _mi_page_abandon _Py__mi_page_abandon
#define _mi_page_empty _Py__mi_page_empty
#define _mi_page_free_collect _Py__mi_page_free_collect
#define _mi_page_free _Py__mi_page_free
#define _mi_page_malloc _Py__mi_page_malloc
#define _mi_page_ptr_unalign _Py__mi_page_ptr_unalign
#define _mi_page_queue_append _Py__mi_page_queue_append
#define _mi_page_reclaim _Py__mi_page_reclaim
#define _mi_page_retire _Py__mi_page_retire
#define _mi_page_unfull _Py__mi_page_unfull
#define _mi_page_use_delayed_free _Py__mi_page_use_delayed_free
#define mi_posix_memalign _Py_mi_posix_memalign
#define _mi_preloading _Py__mi_preloading
#define mi_process_info _Py_mi_process_info
#define mi_process_init _Py_mi_process_init
#define mi_pvalloc _Py_mi_pvalloc
#define _mi_random_init _Py__mi_random_init
#define _mi_random_next _Py__mi_random_next
#define _mi_random_split _Py__mi_random_split
#define mi_realloc_aligned_at _Py_mi_realloc_aligned_at
#define mi_realloc_aligned _Py_mi_realloc_aligned
#define mi_reallocarray _Py_mi_reallocarray
#define mi_reallocf _Py_mi_reallocf
#define mi_reallocn _Py_mi_reallocn
#define mi_realloc _Py_mi_realloc
#define mi_realpath _Py_mi_realpath
#define mi_recalloc_aligned_at _Py_mi_recalloc_aligned_at
#define mi_recalloc_aligned _Py_mi_recalloc_aligned
#define mi_recalloc _Py_mi_recalloc
#define mi_register_deferred_free _Py_mi_register_deferred_free
#define mi_register_error _Py_mi_register_error
#define mi_register_output _Py_mi_register_output
#define mi_reserve_huge_os_pages_at _Py_mi_reserve_huge_os_pages_at
#define mi_reserve_huge_os_pages_interleave _Py_mi_reserve_huge_os_pages_interleave
#define mi_reserve_huge_os_pages _Py_mi_reserve_huge_os_pages
#define mi_reserve_os_memory _Py_mi_reserve_os_memory
#define mi_rezalloc_aligned_at _Py_mi_rezalloc_aligned_at
#define mi_rezalloc_aligned _Py_mi_rezalloc_aligned
#define mi_rezalloc _Py_mi_rezalloc
#define _mi_segment_cache_pop _Py__mi_segment_cache_pop
#define _mi_segment_cache_push _Py__mi_segment_cache_push
#define _mi_segment_huge_page_free _Py__mi_segment_huge_page_free
#define _mi_segment_map_allocated_at _Py__mi_segment_map_allocated_at
#define _mi_segment_map_freed_at _Py__mi_segment_map_freed_at
#define _mi_segment_page_abandon _Py__mi_segment_page_abandon
#define _mi_segment_page_alloc _Py__mi_segment_page_alloc
#define _mi_segment_page_free _Py__mi_segment_page_free
#define _mi_segment_page_start _Py__mi_segment_page_start
#define _mi_segment_thread_collect _Py__mi_segment_thread_collect
#define _mi_stat_counter_increase _Py__mi_stat_counter_increase
#define _mi_stat_decrease _Py__mi_stat_decrease
#define _mi_stat_increase _Py__mi_stat_increase
#define _mi_stats_done _Py__mi_stats_done
#define mi_stats_merge _Py_mi_stats_merge
#define mi_stats_print_out _Py_mi_stats_print_out
#define mi_stats_print _Py_mi_stats_print
#define mi_stats_reset _Py_mi_stats_reset
#define mi_strdup _Py_mi_strdup
#define mi_strndup _Py_mi_strndup
#define mi_thread_done _Py_mi_thread_done
#define mi_thread_init _Py_mi_thread_init
#define mi_thread_stats_print_out _Py_mi_thread_stats_print_out
#define _mi_trace_message _Py__mi_trace_message
#define mi_usable_size _Py_mi_usable_size
#define mi_valloc _Py_mi_valloc
#define _mi_verbose_message _Py__mi_verbose_message
#define mi_version _Py_mi_version
#define _mi_warning_message _Py__mi_warning_message
#define mi_wcsdup _Py_mi_wcsdup
#define mi_wdupenv_s _Py_mi_wdupenv_s
#define mi_zalloc_aligned_at _Py_mi_zalloc_aligned_at
#define mi_zalloc_aligned _Py_mi_zalloc_aligned
#define mi_zalloc _Py_mi_zalloc
#define mi_zalloc_small _Py_mi_zalloc_small
#endif

#define _mi_assert_fail _Py__mi_assert_fail
#define _mi_numa_node_count _Py__mi_numa_node_count
#define _ZSt15get_new_handlerv _Py__ZSt15get_new_handlerv

#include "mimalloc.h"
#include "mimalloc-internal.h"

#endif /* !Py_INTERNAL_MIMALLOC_H */
