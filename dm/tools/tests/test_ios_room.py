# The residency-commit room policy in nvn_mtl.m (mem_bytes_to_free, ios_effective_headroom).
# The two jetsam logs are replayed as back-to-back adds while os_proc_available_memory has not moved yet.
# A purge only counts up to the bytes the log showed were still packaged and idle enough to discard.
import unittest

MB = 1 << 20
KEEP = 600 * MB
DANGER = 300 * MB
TIGHT = 1200 * MB
FLOOR = 1536 * MB


def bytes_to_free(headroom, incoming):
    if headroom < DANGER:
        return KEEP + incoming - headroom if KEEP + incoming > headroom else 0
    if headroom >= TIGHT and incoming < 64 * MB:
        return 0
    if headroom < FLOOR and headroom < KEEP + incoming:
        return KEEP + incoming - headroom
    if incoming >= 128 * MB and headroom < KEEP + incoming:
        return KEEP + incoming - headroom
    return 0


def burst(headroom, adds, purgeable):
    """One sample of headroom, then each add before the kernel reports a new sample. Returns (freed, lowest headroom after an add)."""
    sample, charged, freed_total, lowest = headroom, 0, 0, headroom
    for incoming in adds:
        eff = sample - charged if sample > charged else 0
        need = bytes_to_free(eff, incoming)
        got = min(need, max(0, purgeable - freed_total))
        freed_total += got
        if got >= incoming:
            extra = got - incoming
            charged = charged - extra if charged > extra else 0
        else:
            charged += incoming - got
        after = sample - charged if sample > charged else 0
        lowest = min(lowest, after)
    return freed_total, lowest


class Room(unittest.TestCase):
    def test_policy_edges(self):
        self.assertEqual(bytes_to_free(2000 * MB, 32 * MB), 0)
        self.assertEqual(bytes_to_free(5000 * MB, 200 * MB), 0)
        self.assertEqual(bytes_to_free(250 * MB, 0), 350 * MB)
        self.assertEqual(bytes_to_free(1018 * MB, 390 * MB), 0)  # first add of the 1938 burst still fits above 600

    def test_1938_burst_with_the_packaged_set(self):
        # crash_1938: headroom 1018 MB, then four residency commits, 389.7 + 310.1 + 279.0 + 298.4 MB.
        # Packaged textures were 1812 MB. The old 8 s idle had marked only ~474 MB of that (858 of 3278).
        adds = [int(x * MB) for x in (389.7, 310.1, 279.0, 298.4)]
        freed, lowest = burst(1018 * MB, adds, 1812 * MB)
        self.assertGreaterEqual(lowest, 600 * MB)
        self.assertLess(freed, 1812 * MB)
        _, stuck = burst(1018 * MB, adds, 474 * MB)
        self.assertLess(stuck, 300 * MB)  # what 529f70ae could purge: the copies already marked, and not between these commits

    def test_1906_jump_with_already_discardable(self):
        # crash_1906: headroom 918 MB, GPU allocated already up 668 MB in that sample, packaged 1561 MB of which the
        # volatile mark covered about 1066 MB (1897 of 2779) but had not freed it.
        adds = [668 * MB]
        freed, lowest = burst(918 * MB, adds, 1066 * MB)
        self.assertGreaterEqual(lowest, 600 * MB)
        self.assertGreater(freed, 0)
