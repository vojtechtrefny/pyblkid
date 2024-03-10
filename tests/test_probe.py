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

        cls.ver_code, _version, _date = blkid.get_library_version()

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

        if self.ver_code >= 2300:
            pr.sector_size = 4096
            self.assertEqual(pr.sector_size, 4096)
        else:
            with self.assertRaises(AttributeError):
                pr.sector_size = 4096

        pr.reset_probe()

    def test_probing(self):
        pr = blkid.Probe()

        with self.assertRaises(ValueError):
            pr.do_probe()

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

        if hasattr(pr, "reset_buffers"):
            pr.reset_buffers()

        pr.step_back()
        ret = pr.do_probe()
        self.assertTrue(ret)

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        if hasattr(pr, "hide_range"):
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

        # not probed yet, len should be 0
        self.assertEqual(len(pr), 0)
        self.assertFalse(pr.keys())
        self.assertFalse(pr.values())
        self.assertFalse(pr.items())

        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        # three or more items should be in the probe now
        self.assertGreaterEqual(len(pr), 3)

        usage = pr.lookup_value("USAGE")
        self.assertEqual(usage, b"filesystem")

        usage = pr["USAGE"]
        self.assertEqual(usage, b"filesystem")

        fstype = pr.lookup_value("TYPE")
        self.assertEqual(fstype, b"ext3")

        fstype = pr["TYPE"]
        self.assertEqual(fstype, b"ext3")

        fsuuid = pr.lookup_value("UUID")
        self.assertEqual(fsuuid, b"35f66dab-477e-4090-a872-95ee0e493ad6")

        fsuuid = pr["UUID"]
        self.assertEqual(fsuuid, b"35f66dab-477e-4090-a872-95ee0e493ad6")

        keys = pr.keys()
        self.assertIn("USAGE", keys)
        self.assertIn("TYPE", keys)
        self.assertIn("UUID", keys)

        values = pr.values()
        self.assertIn("filesystem", values)
        self.assertIn("ext3", values)
        self.assertIn("35f66dab-477e-4090-a872-95ee0e493ad6", values)

        items = pr.items()
        self.assertIn(("USAGE", "filesystem"), items)
        self.assertIn(("TYPE", "ext3"), items)
        self.assertIn(("UUID", "35f66dab-477e-4090-a872-95ee0e493ad6"), items)

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

    def test_topology(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_superblocks(True)
        pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_UUID)

        pr.enable_topology(True)

        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        self.assertIsNotNone(pr.topology)

        self.assertEqual(pr.topology.alignment_offset, 0)
        self.assertEqual(pr.topology.logical_sector_size, 512)
        self.assertEqual(pr.topology.minimum_io_size, 512)
        self.assertEqual(pr.topology.optimal_io_size, 0)
        self.assertEqual(pr.topology.physical_sector_size, 512)

        if self.ver_code >= 2360:
            self.assertFalse(pr.topology.dax)
        else:
            with self.assertRaises(AttributeError):
                self.assertIsNone(pr.topology.dax)


@unittest.skipUnless(os.geteuid() == 0, "requires root access")
class WipeTestCase(unittest.TestCase):

    test_image = "test.img.xz"
    loop_dev = None

    def setUp(self):
        test_dir = os.path.abspath(os.path.dirname(__file__))
        self.loop_dev = utils.loop_setup(os.path.join(test_dir, self.test_image))

    def tearDown(self):
        test_dir = os.path.abspath(os.path.dirname(__file__))
        if self.loop_dev:
            utils.loop_teardown(self.loop_dev,
                                filename=os.path.join(test_dir, self.test_image))

    def test_wipe(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev, flags=os.O_RDWR)

        pr.enable_superblocks(True)
        pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_MAGIC)

        while pr.do_probe():
            pr.do_wipe(False)

        pr.reset_probe()

        ret = pr.do_probe()
        self.assertFalse(ret)


if __name__ == "__main__":
    unittest.main()
