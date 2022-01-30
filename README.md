# pyblkid

Python bindings for libblkid library.

```python
pr = blkid.Probe()
pr.set_device("/dev/sda1")

pr.enable_superblocks(True)
pr.set_superblocks_flags(blkid.SUBLKS_TYPE | blkid.SUBLKS_USAGE | blkid.SUBLKS_UUID)

pr.do_safeprobe()

# print usage
pr["USAGE"]
```