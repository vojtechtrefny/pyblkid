# pylibblkid

[![PyPI version](https://badge.fury.io/py/pylibblkid.svg)](https://badge.fury.io/py/pylibblkid)

Python bindings for libblkid library.

```python
import blkid

pr = blkid.Probe()
pr.set_device("/dev/sda1")

pr.enable_superblocks(True)
pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_UUID)

pr.do_safeprobe()

# print usage
pr["USAGE"]
```
