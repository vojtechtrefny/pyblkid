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
