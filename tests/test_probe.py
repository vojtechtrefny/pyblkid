import os
import unittest

from . import utils

import blkid


@unittest.skipUnless(os.geteuid() == 0, "requires root access")
class ProbeTestCase(unittest.TestCase):

    test_image = "test.img.xz"
    loop_dev = None

    @classmethod
    def setUpClass(cls):
        test_dir = os.path.abspath(os.path.dirname(__file__))
        cls.loop_dev = utils.loop_setup(os.path.join(test_dir, cls.test_image))

    @classmethod
    def tearDownClass(cls):
        if cls.loop_dev:
            utils.loop_teardown(cls.loop_dev)

    def test_probe(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        self.assertEqual(pr.offset, 0)
        self.assertEqual(pr.sectors, 4096)
        self.assertEqual(pr.sector_size, 512)
        self.assertEqual(pr.size, pr.sectors * pr.sector_size)

        self.assertGreater(pr.fd, 0)
        self.assertNotEqual(pr.devno, 0)
        self.assertNotEqual(pr.wholedisk_devno, 0)

        self.assertTrue(pr.is_wholedisk)

        pr.sector_size = 4096
        self.assertEqual(pr.sector_size, 4096)

        pr.reset_probe()

    def test_probing(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_superblocks(True)
        pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_MAGIC)

        ret = pr.do_probe()
        self.assertTrue(ret)

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        pr.step_back()
        ret = pr.do_probe()
        self.assertTrue(ret)

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        pr.reset_buffers()
        pr.step_back()
        ret = pr.do_probe()
        self.assertTrue(ret)

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        offset = pr.lookup_value("SBMAGIC_OFFSET")
        magic = pr.lookup_value("SBMAGIC")
        pr.hide_range(int(offset), len(magic))

        pr.step_back()
        ret = pr.do_probe()
        self.assertFalse(ret)

        with self.assertRaises(RuntimeError):
            usage = pr.lookup_value("USAGE")

    def test_safe_probing(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_superblocks(True)
        pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_UUID)

        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        fstype = pr.lookup_value("TYPE")
        self.assertEqual(fstype, b"ext3")

        fsuuid = pr.lookup_value("UUID")
        self.assertEqual(fsuuid, b"35f66dab-477e-4090-a872-95ee0e493ad6")

    def test_probe_filter_type(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_superblocks(True)
        pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_UUID)

        pr.filter_superblocks_type(blkid.FLTR_ONLYIN, ["ext3", "ext4"])
        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        fstype = pr.lookup_value("TYPE")
        self.assertEqual(fstype, b"ext3")

        pr.filter_superblocks_type(blkid.FLTR_NOTIN, ["ext3", "ext4"])
        ret = pr.do_safeprobe()
        self.assertFalse(ret)

        with self.assertRaises(RuntimeError):
            fstype = pr.lookup_value("TYPE")

        pr.filter_superblocks_type(blkid.FLTR_NOTIN, ["vfat", "ntfs"])
        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        fstype = pr.lookup_value("TYPE")
        self.assertEqual(fstype, b"ext3")

        # invert the filter
        pr.invert_superblocks_filter()
        ret = pr.do_safeprobe()
        self.assertFalse(ret)

        with self.assertRaises(RuntimeError):
            fstype = pr.lookup_value("TYPE")

        # reset to default
        pr.reset_superblocks_filter()
        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        fstype = pr.lookup_value("TYPE")
        self.assertEqual(fstype, b"ext3")

    def test_probe_filter_usage(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_superblocks(True)
        pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_UUID)

        pr.filter_superblocks_usage(blkid.FLTR_ONLYIN, blkid.USAGE_FILESYSTEM)
        pr.do_safeprobe()

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        pr.filter_superblocks_usage(blkid.FLTR_NOTIN, blkid.USAGE_FILESYSTEM | blkid.USAGE_CRYPTO)
        pr.do_safeprobe()

        with self.assertRaises(RuntimeError):
            usage = pr.lookup_value("USAGE")

        pr.filter_superblocks_usage(blkid.FLTR_NOTIN, blkid.USAGE_RAID | blkid.USAGE_CRYPTO)
        pr.do_safeprobe()

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        # invert the filter
        pr.invert_superblocks_filter()
        pr.do_safeprobe()

        with self.assertRaises(RuntimeError):
            usage = pr.lookup_value("USAGE")

        # reset to default
        pr.reset_superblocks_filter()
        pr.do_safeprobe()

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")


if __name__ == "__main__":
    unittest.main()
