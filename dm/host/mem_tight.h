// Devices that cannot keep this game's ordinary working set resident. iOS: the room to the jetsam limit
// (os_proc_available_memory) is under 9 GB, as on an 8 GB iPad (M2, about 7.96 GB at start) and not on a 12 GB iPhone.
// The Mac has no such limit (room 0): physical RAM under 9 GB, an 8 GB M1 MacBook Air, is the same case, and 16 GB is not.
// The chip's 8 GB is not free for the game. iPadOS and macOS already hold about 2 GB (the jetsam limit sits beside the
// system on the iPad; on the Mac the system is in the same pool), so the caps below are sized for what is left.
// DM_MEM_TIGHT=1 or 0 forces the switch. Each policy has its own key and, when that key is unset, follows this.
#ifndef DM_MEM_TIGHT_H
#define DM_MEM_TIGHT_H
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

#define DM_MEM_TIGHT_ROOM ((uint64_t)9 << 30)   // under this: the new limits. 9 GB itself keeps the ordinary ones
#define DM_MEM_OS_HOLD ((uint64_t)2 << 30)      // RAM the system is already using on an 8 GB iPad or MacBook Air; not given to the game

static inline uint64_t dm_phys_ram(void) {
#if defined(__APPLE__)
    uint64_t b = 0; size_t n = sizeof b;
    if (sysctlbyname("hw.memsize", &b, &n, NULL, 0)) return 0;
    return b;
#else
    return 0;
#endif
}
// force: DM_MEM_TIGHT when set. room: jetsam headroom, 0 on the Mac. phys: hw.memsize, used only when room is 0
static inline int dm_mem_tight_room(const char *force, uint64_t room, uint64_t phys) {
    if (force && *force) return atoi(force) > 0;
    if (room) return room < DM_MEM_TIGHT_ROOM;
    return phys && phys < DM_MEM_TIGHT_ROOM;
}
// A size switch: bytes, or whole K / M / G. 0 when unset or not a size
static inline uint64_t dm_env_size(const char *s) {
    if (!s || !*s) return 0;
    char *end; unsigned long long v = strtoull(s, &end, 0); int sh = 0;
    if (*end == 'G' || *end == 'g') sh = 30;
    else if (*end == 'M' || *end == 'm') sh = 20;
    else if (*end == 'K' || *end == 'k') sh = 10;
    if (sh) end++;
    if (*end || (sh && v > (~0ull >> sh))) return 0;
    return (uint64_t)v << sh;
}
// Cap on the speculative texture walk (nvn_mtl.m pool_budget), above what shaders have already asked for. 0: no cap,
// the historical recommendedMaxWorkingSetSize/20. Unset on a tight device: 1536 MB, so the walk does not fill the
// rest of the 8 GB the system is sharing
static inline uint64_t dm_gpu_budget_cap(int tight, const char *env) {
    if (env && *env) return (!strcmp(env, "0") || !strcmp(env, "off")) ? 0 : dm_env_size(env);
    return tight ? (1536ull << 20) : 0;
}
// Recycled render-target heaps (rh_trim). Unset follows tight
static inline int dm_gpu_purge(int tight, const char *env) { return env && *env ? atoi(env) > 0 : tight; }
static inline uint64_t dm_gpu_heap_cap(int purge) { return purge ? (32ull << 20) : (256ull << 20); }
static inline uint64_t dm_gpu_heap_idle_ns(int purge) { return purge ? 1000000000ull : 3000000000ull; }
// Bytes of pipeline archive to map. UINT64_MAX: every file, as before. Unset on a tight device: 256 MB of the newest files.
// The rest stay on disk and are not mapped; a multi-gigabyte archive would otherwise sit in the RAM the system needs
static inline uint64_t dm_pso_archive_max(int tight, const char *env) {
    if (env && *env) {
        if (!strcmp(env, "0") || !strcmp(env, "off")) return UINT64_MAX;
        uint64_t n = dm_env_size(env); return n ? n : UINT64_MAX;
    }
    return tight ? (256ull << 20) : UINT64_MAX;
}
// MADV_RANDOM on guest file mappings. Unset follows tight (and only applies once the guest is in files)
static inline int dm_guest_file_advise(int tight, const char *env) { return env && *env ? atoi(env) > 0 : tight; }
// Ocarina of Time's dynamic-resolution levels 0..9 run from 0.75 to 0.333 of the output (its note). Full size is level 0.
// DM_RENDER_SCALE is a fraction of that full size. The level is the nearest whose scale is 0.75 * scale, and not a finer one.
// 1 or an unset 0: level 0, the game's own size
static inline unsigned dm_oot_render_floor(float scale) {
    if (!(scale < 1.f) || !(scale > 0.f)) return 0;
    int n = (int)((1.f - scale) * 16.2f + 0.5f);   // (0.75 - 0.333) / 9 ≈ 0.0463; 0.75 / 0.0463 ≈ 16.2
    if (n < 0) n = 0;
    if (n > 9) n = 9;
    return (unsigned)n;
}
#endif
