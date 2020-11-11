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

    def test_cache(cls):
        cache = blkid.Cache(filename=cls.cache_file)
        cache.probe_all()
        cache.probe_all(removable=True)

if __name__ == "__main__":
    unittest.main()
