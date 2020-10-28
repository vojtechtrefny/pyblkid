import os
import unittest

from . import utils

import blkid


class BlkidTestCase(unittest.TestCase):

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

    def test_blkid(self):
        self.assertTrue(blkid.known_fstype("ext4"))
        self.assertFalse(blkid.known_fstype("not-a-filesystem"))

    def test_uevent(self):
        with self.assertRaises(RuntimeError):
            blkid.send_uevent("not-a-device", "change")

        blkid.send_uevent(self.loop_dev, "change")

    def test_devname(self):
        sysfs_path = "/sys/block/%s/dev" % os.path.basename(self.loop_dev)
        major_minor = utils.read_file(sysfs_path).strip()
        major, minor = major_minor.split(":")
        devno = os.makedev(int(major), int(minor))

        devpath = blkid.devno_to_devname(devno)
        self.assertEqual(devpath, self.loop_dev)
