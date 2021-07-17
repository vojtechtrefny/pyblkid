import os
import unittest
import tempfile

from . import utils

import blkid


@unittest.skipUnless(os.geteuid() == 0, "requires root access")
class CacheTestCase(unittest.TestCase):

    test_image = "test.img.xz"
    loop_dev = None
    cache_file = None

    @classmethod
    def setUpClass(cls):
        test_dir = os.path.abspath(os.path.dirname(__file__))
        cls.loop_dev = utils.loop_setup(os.path.join(test_dir, cls.test_image))

        _, cls.cache_file = tempfile.mkstemp()

    @classmethod
    def tearDownClass(cls):
        if cls.loop_dev:
            utils.loop_teardown(cls.loop_dev)

        if cls.cache_file:
            os.remove(cls.cache_file)

    def test_cache(self):
        cache = blkid.Cache(filename=self.cache_file)
        cache.probe_all()
        cache.probe_all(removable=True)
        cache.gc()

        device = cache.get_device(self.loop_dev)
        self.assertIsNotNone(device)
        self.assertEqual(device.devname, self.loop_dev)

        device = cache.find_device("LABEL", "not-in-cache")
        self.assertIsNone(device)

        device = cache.find_device("LABEL", "test-ext3")
        self.assertIsNotNone(device)
        self.assertEqual(device.devname, self.loop_dev)

        self.assertIsNotNone(device.tags)
        self.assertIn("UUID", device.tags.keys())
        self.assertEqual(device.tags["UUID"], "35f66dab-477e-4090-a872-95ee0e493ad6")
        self.assertIn("LABEL", device.tags.keys())
        self.assertEqual(device.tags["LABEL"], "test-ext3")
        self.assertIn("TYPE", device.tags.keys())
        self.assertEqual(device.tags["TYPE"], "ext3")

        self.assertTrue(cache.devices)
        self.assertIn(self.loop_dev, [d.devname for d in cache.devices])

        device.verify()
        self.assertIsNotNone(device)
        self.assertEqual(device.devname, self.loop_dev)

if __name__ == "__main__":
    unittest.main()
