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

        self.assertTrue(blkid.known_pttype("dos"))
        self.assertFalse(blkid.known_fstype("not-a-partition-table"))

        self.assertEqual(blkid.parse_version_string("2.16.0"), 2160)

        code, version, date = blkid.get_library_version()
        self.assertGreater(code, 0)
        self.assertIsNotNone(version)
        self.assertIsNotNone(date)

        ttype, tvalue = blkid.parse_tag_string("NAME=test")
        self.assertEqual(ttype, "NAME")
        self.assertEqual(tvalue, "test")

        size = blkid.get_dev_size(self.loop_dev)
        self.assertEqual(size, 2097152)  # test.img is 2 MiB

        # dos should be always supported so we can use it here to test
        types = blkid.partition_types()
        self.assertIn("dos", types)

        # ext4 should be always supported so we can use it here to test
        supers = blkid.superblocks()
        self.assertIn("ext4", supers)

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

    def test_safe_encode_string(self):
        string = "aaaaaa"
        safe_string = blkid.safe_string(string)
        encoded_string = blkid.encode_string(string)
        self.assertEqual(string, safe_string)
        self.assertEqual(string, encoded_string)

        string = "aa aaa"
        safe_string = blkid.safe_string(string)
        encoded_string = blkid.encode_string(string)
        self.assertEqual(safe_string, "aa_aaa")
        self.assertEqual(encoded_string, "aa\\x20aaa")
