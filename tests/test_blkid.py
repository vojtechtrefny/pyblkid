import os
import unittest

from . import utils

import blkid


class BlkidTestCase(unittest.TestCase):

    def test_blkid(self):
        self.assertTrue(blkid.known_fstype("ext4"))
        self.assertFalse(blkid.known_fstype("not-a-filesystem"))
