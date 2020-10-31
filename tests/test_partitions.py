import os
import unittest

from . import utils

import blkid


@unittest.skipUnless(os.geteuid() == 0, "requires root access")
class PartitionsTestCase(unittest.TestCase):

    test_image = "gpt.img.xz"
    loop_dev = None

    @classmethod
    def setUpClass(cls):
        test_dir = os.path.abspath(os.path.dirname(__file__))
        cls.loop_dev = utils.loop_setup(os.path.join(test_dir, cls.test_image))

    @classmethod
    def tearDownClass(cls):
        if cls.loop_dev:
            utils.loop_teardown(cls.loop_dev)

    def test_partlist(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_partitions(True)

        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        plist = pr.partitions
        self.assertEqual(plist.numof_partitions, 5)

    def test_partitions_filter(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_partitions(True)

        ret = pr.do_safeprobe()
        self.assertTrue(ret)
        self.assertEqual(pr.partitions.numof_partitions, 5)

        pr.filter_partitions_type(blkid.FLTR_ONLYIN, ["gpt"])
        ret = pr.do_safeprobe()
        self.assertTrue(ret)
        self.assertEqual(pr.partitions.numof_partitions, 5)

        pr.filter_partitions_type(blkid.FLTR_ONLYIN, ["gpt", "dos"])
        ret = pr.do_safeprobe()
        self.assertTrue(ret)
        self.assertEqual(pr.partitions.numof_partitions, 5)

        pr.filter_partitions_type(blkid.FLTR_NOTIN, ["gpt"])
        ret = pr.do_safeprobe()
        self.assertFalse(ret)
        with self.assertRaises(RuntimeError):
            pr.partitions

        pr.invert_partitions_filter()
        ret = pr.do_safeprobe()
        self.assertTrue(ret)
        self.assertEqual(pr.partitions.numof_partitions, 5)

        pr.filter_partitions_type(blkid.FLTR_NOTIN, ["gpt"])
        ret = pr.do_safeprobe()
        self.assertFalse(ret)
        with self.assertRaises(RuntimeError):
            pr.partitions

        pr.reset_partitions_filter()
        ret = pr.do_safeprobe()
        self.assertTrue(ret)
        self.assertEqual(pr.partitions.numof_partitions, 5)

    def test_partition_table(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_partitions(True)

        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        self.assertIsNotNone(pr.partitions)
        self.assertIsNotNone(pr.partitions.table)

        self.assertEqual(pr.partitions.table.type, "gpt")
        self.assertEqual(pr.partitions.table.id, "dd27f98d-7519-4c9e-8041-f2bfa7b1ef61")
        self.assertEqual(pr.partitions.table.offset, 512)

        nested = pr.partitions.table.get_parent()
        self.assertIsNone(nested)

    def test_partition(self):
        pr = blkid.Probe()
        pr.set_device(self.loop_dev)

        pr.enable_partitions(True)

        ret = pr.do_safeprobe()
        self.assertTrue(ret)

        self.assertIsNotNone(pr.partitions)

        part = pr.partitions.get_partition(0)
        self.assertEqual(part.type, 0)
        self.assertEqual(part.type_string, "ebd0a0a2-b9e5-4433-87c0-68b6b72699c7")
        self.assertEqual(part.uuid, "1dcf10bc-637e-4c52-8203-087ae10a820b")
        self.assertTrue(part.is_primary)
        self.assertFalse(part.is_extended)
        self.assertFalse(part.is_logical)
        self.assertEqual(part.name, "ThisIsName")
        self.assertEqual(part.flags, 0)
        self.assertEqual(part.partno, 1)
        self.assertEqual(part.start, 34)
        self.assertEqual(part.size, 2014)
