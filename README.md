# pylibblkid

[![PyPI version](https://badge.fury.io/py/pylibblkid.svg)](https://badge.fury.io/py/pylibblkid)

Python bindings for libblkid library.

## Usage examples

### Probing a device
```python
import blkid

pr = blkid.Probe()
pr.set_device("/dev/sda1")

pr.enable_superblocks(True)
pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_UUID)

pr.do_safeprobe()

# print device properties as a dictionary
print(dict(pr))
```

### Searching for device with specified label
```python
import blkid

cache = blkid.Cache()
cache.probe_all()

dev = cache.find_device("LABEL", "mylabel")

# if found print found device and its properties
if dev:
    print(dev.devname)
    print(dev.tags)
```
