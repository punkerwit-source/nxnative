// The tight-device rules (dm/host/mem_tight.h): an 8 GB iPad's jetsam room, an 8 GB Mac's physical RAM, and not a 12 GB phone or a 16 GB Mac
#include "../mem_tight.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
    const uint64_t G = 1ull << 30;
    assert(!dm_mem_tight_room(NULL, 0, 0));
    assert(!dm_mem_tight_room(NULL, 12 * G, 12 * G));
    assert(!dm_mem_tight_room(NULL, 9 * G, 8 * G));          // 9 GB of room keeps the ordinary limits, even if RAM is 8
    assert(dm_mem_tight_room(NULL, (8ull << 30) - (40ull << 20), 8 * G));   // the iPad's 7.96 GB
    assert(!dm_mem_tight_room(NULL, 0, 16 * G));             // a 16 GB Mac
    assert(dm_mem_tight_room(NULL, 0, 8 * G));               // an 8 GB M1: no jetsam room, physical RAM
    assert(!dm_mem_tight_room("0", 8 * G, 8 * G));
    assert(dm_mem_tight_room("1", 16 * G, 16 * G));
    assert(DM_MEM_OS_HOLD == (2ull << 30));
    assert(dm_gpu_budget_cap(0, NULL) == 0);
    assert(dm_gpu_budget_cap(1, NULL) == (1536ull << 20));   // leaves the system's share of an 8 GB machine
    assert(dm_gpu_budget_cap(1, "0") == 0 && dm_gpu_budget_cap(0, "1536M") == (1536ull << 20));
    assert(dm_gpu_heap_cap(0) == (256ull << 20) && dm_gpu_heap_cap(1) == (32ull << 20));
    assert(dm_gpu_heap_idle_ns(0) == 3000000000ull && dm_gpu_heap_idle_ns(1) == 1000000000ull);
    assert(!dm_gpu_purge(0, NULL) && dm_gpu_purge(1, NULL) && !dm_gpu_purge(1, "0") && dm_gpu_purge(0, "1"));
    assert(dm_pso_archive_max(0, NULL) == UINT64_MAX);
    assert(dm_pso_archive_max(1, NULL) == (256ull << 20));
    assert(dm_pso_archive_max(1, "off") == UINT64_MAX && dm_pso_archive_max(0, "1G") == G);
    assert(!dm_guest_file_advise(0, NULL) && dm_guest_file_advise(1, NULL) && !dm_guest_file_advise(1, "0"));
    assert(dm_oot_render_floor(1.f) == 0 && dm_oot_render_floor(0.f) == 0);
    assert(dm_oot_render_floor(0.75f) == 4);                 // about 0.75 of full size
    assert(dm_oot_render_floor(0.5f) == 8);
    assert(dm_oot_render_floor(0.67f) == 5);
    printf("t_mem_tight: ok\n");
    return 0;
}
