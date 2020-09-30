import os
import subprocess


def run_command(command):
    res = subprocess.Popen(command, shell=True, stdout=subprocess.PIPE,
                           stderr=subprocess.PIPE)

    out, err = res.communicate()
    if res.returncode != 0:
        output = out.decode().strip() + "\n\n" + err.decode().strip()
    else:
        output = out.decode().strip()
    return (res.returncode, output)


def loop_setup(filename):
    if filename.endswith(".xz") and not os.path.exists(filename[:-3]):
        ret, out = run_command("xz --decompress --keep %s" % filename)
        if ret != 0:
            raise RuntimeError("Failed to decompress file %s: %s" % (filename, out))
    filename = filename[:-3]

    ret, out = run_command("losetup --show -f %s" % filename)
    if ret != 0:
        raise RuntimeError("Failed to create loop device from %s: %s" % (filename, out))
    return out


def loop_teardown(loopdev):
    ret, out = run_command("losetup -d %s" % loopdev)
    if ret != 0:
        raise RuntimeError("Failed to detach loop device %s: %s" % (loopdev, out))
