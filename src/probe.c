/*
 * Copyright (C) 2020  Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "probe.h"

#include <blkid/blkid.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#define UNUSED __attribute__((unused))


PyObject *Probe_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    ProbeObject *self = (ProbeObject*) type->tp_alloc (type, 0);

    if (self) {
        self->probe = NULL;
        self->fd = -1;
    }

    return (PyObject *) self;
}

int Probe_init (ProbeObject *self, PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    if (self->probe)
        blkid_free_probe (self->probe);

    self->probe = blkid_new_probe ();
    if (!self->probe) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create new Probe.");
        return -1;
    }

    return 0;
}

void Probe_dealloc (ProbeObject *self) {
    if (!self->probe)
        /* if init fails */
        return;

    if (self->fd > 0)
        close (self->fd);

    blkid_free_probe (self->probe);
    Py_TYPE (self)->tp_free ((PyObject *) self);
}

PyDoc_STRVAR(Probe_set_device__doc__,
"set_device (device, flags=os.O_RDONLY|os.O_CLOEXEC, offset=0, size=0)\n\n"
"Assigns the device to probe control struct, resets internal buffers and resets the current probing.\n\n"
"'flags' define flags for the 'open' system call. By default the device will be opened as read-only.\n"
"'offset' and 'size' specify begin and size of probing area (zero means whole device/file)");
static PyObject *Probe_set_device (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    char *kwlist[] = { "device", "flags", "offset", "size", NULL };
    char *device = NULL;
    blkid_loff_t offset = 0;
    blkid_loff_t size = 0;
    int flags = O_RDONLY|O_CLOEXEC;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s|iKK", kwlist, &device, &flags, &offset, &size)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    self->fd = open (device, flags);
    if (self->fd == -1) {
        PyErr_Format (PyExc_OSError, "Failed to open device '%s': %s", device, strerror (errno));
        return NULL;
    }

    ret = blkid_probe_set_device (self->probe, self->fd, offset, size);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to set device");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_enable_superblocks__doc__,
"enable_superblocks (enable)\n\n" \
"Enables/disables the superblocks probing for non-binary interface.");
static PyObject *Probe_enable_superblocks (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    bool enable = false;
    char *kwlist[] = { "enable", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "p", kwlist, &enable)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_probe_enable_superblocks (self->probe, enable);
    if (ret != 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to %s superblocks probing", enable ? "enable" : "disable");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_set_superblocks_flags__doc__,
"set_superblocks_flags (flags)\n\n" \
"Sets probing flags to the superblocks prober. This function is optional, the default are blkid.SUBLKS_DEFAULTS flags.\n"
"Use blkid.SUBLKS_* constants for the 'flags' argument.");
static PyObject *Probe_set_superblocks_flags (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    int flags = 0;
    char *kwlist[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &flags)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_probe_set_superblocks_flags (self->probe, flags);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to set partition flags");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_filter_superblocks_type__doc__,
"filter_superblocks_type (flag, names)\n\n" \
"Filter superblocks prober results based on type.\n"
"blkid.FLTR_NOTIN - probe for all items which are NOT IN names\n"
"blkid.FLTR_ONLYIN - probe for items which are IN names\n"
"names: array of probing function names (e.g. 'vfat').");
static PyObject *Probe_filter_superblocks_type (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    int flag = 0;
    PyObject *pynames = NULL;
    PyObject *pystring = NULL;
    Py_ssize_t len = 0;
    char **names = NULL;
    char *kwlist[] = { "flag", "names", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "iO", kwlist, &flag, &pynames)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    if (!PySequence_Check (pynames)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse list of names for filter");
        return NULL;
    }

    len = PySequence_Size (pynames);
    if (len < 1) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse list of names for filter");
        return NULL;
    }

    names = malloc(sizeof (char *) * (len + 1));

    for (Py_ssize_t i = 0; i < len; i++) {
        pystring = PyUnicode_AsEncodedString (PySequence_GetItem (pynames, i), "utf-8", "replace");
        names[i] = strdup (PyBytes_AsString (pystring));
        Py_DECREF (pystring);
    }
    names[len] = NULL;

    ret = blkid_probe_filter_superblocks_type (self->probe, flag, names);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to set probe filter");
        for (Py_ssize_t i = 0; i < len; i++)
            free(names[i]);
        free (names);
        return NULL;
    }

    for (Py_ssize_t i = 0; i < len; i++)
            free(names[i]);
    free (names);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_filter_superblocks_usage__doc__,
"filter_superblocks_usage (flag, usage)\n\n" \
"Filter superblocks prober results based on usage.\n"
"blkid.FLTR_NOTIN - probe for all items which are NOT IN names\n"
"blkid.FLTR_ONLYIN - probe for items which are IN names\n"
"usage: blkid.USAGE_* flags");
static PyObject *Probe_filter_superblocks_usage (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    int flag = 0;
    int usage = 0;
    char *kwlist[] = { "flag", "usage", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, &flag, &usage)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_probe_filter_superblocks_usage (self->probe, flag, usage);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to set probe filter");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_invert_superblocks_filter__doc__,
"invert_superblocks_filter ()\n\n"
"This function inverts superblocks probing filter.\n");
static PyObject *Probe_invert_superblocks_filter (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_probe_invert_superblocks_filter (self->probe);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to invert superblock probing filter");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_reset_superblocks_filter__doc__,
"reset_superblocks_filter ()\n\n"
"This function resets superblocks probing filter.\n");
static PyObject *Probe_reset_superblocks_filter (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_probe_reset_superblocks_filter (self->probe);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to reset superblock probing filter");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_enable_partitions__doc__,
"enable_partitions (enable)\n\n" \
"Enables/disables the partitions probing for non-binary interface.");
static PyObject *Probe_enable_partitions (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    bool enable = false;
    char *kwlist[] = { "enable", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "p", kwlist, &enable)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_probe_enable_partitions (self->probe, enable);
    if (ret != 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to %s partitions probing", enable ? "enable" : "disable");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_set_partitions_flags__doc__,
"set_partitions_flags (flags)\n\n" \
"Sets probing flags to the partitions prober. This function is optional.\n"
"Use blkid.PARTS_* constants for the 'flags' argument.");
static PyObject *Probe_set_partitions_flags (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    int flags = 0;
    char *kwlist[] = { "flags", NULL };

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i", kwlist, &flags)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_probe_set_partitions_flags (self->probe, flags);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to set superblock flags");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_lookup_value__doc__,
"lookup_value (name)\n\n" \
"Assigns the device to probe control struct, resets internal buffers and resets the current probing.");
static PyObject *Probe_lookup_value (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    char *kwlist[] = { "name", NULL };
    char *name = NULL;
    const char *value = NULL;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &name)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_probe_lookup_value (self->probe, name, &value, NULL);
    if (ret != 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to lookup '%s'", name);
        return NULL;
    }

    return PyBytes_FromString (value);
}

PyDoc_STRVAR(Probe_do_safeprobe__doc__,
"do_safeprobe ()\n\n"
"This function gathers probing results from all enabled chains and checks for ambivalent results"
"(e.g. more filesystems on the device).\n\n"
"Note about superblocks chain -- the function does not check for filesystems when a RAID signature is detected.\n"
"The function also does not check for collision between RAIDs. The first detected RAID is returned.\n"
"The function checks for collision between partition table and RAID signature -- it's recommended to "
"enable partitions chain together with superblocks chain.\n");
static PyObject *Probe_do_safeprobe (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_do_safeprobe (self->probe);
    if (ret < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to safeprobe the device");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_do_fullprobe__doc__,
"do_fullprobe ()\n\n"
"This function gathers probing results from all enabled chains. Same as do_safeprobe() but "
"does not check for collision between probing result.");
static PyObject *Probe_do_fullprobe (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_do_fullprobe (self->probe);
    if (ret < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to fullprobe the device");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_do_probe__doc__,
"do_probe ()\n\n"
"Calls probing functions in all enabled chains. The superblocks chain is enabled by default."
"The do_probe() stores result from only one probing function. It's necessary to call this routine "
"in a loop to get results from all probing functions in all chains. The probing is reset by "
"reset_probe() or by filter functions.");
static PyObject *Probe_do_probe (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_do_probe (self->probe);
    if (ret < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to probe the device");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_step_back__doc__,
"step_back ()\n\n"
"This function move pointer to the probing chain one step back -- it means that the previously "
"used probing function will be called again in the next Probe.do_probe() call.\n"
"This is necessary for example if you erase or modify on-disk superblock according to the "
"current libblkid probing result.\n"
"Note that Probe.hide_range() changes semantic of this function and cached buffers are "
"not reset, but library uses in-memory modified buffers to call the next probing function.");
static PyObject *Probe_step_back (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_probe_step_back (self->probe);
    if (ret < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to step back the probe");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_reset_buffers__doc__,
"reset_buffers ()\n\n"
"libblkid reuse all already read buffers from the device. The buffers may be modified by Probe.hide_range().\n"
"This function reset and free all cached buffers. The next Probe.do_probe() will read all data from the device.");
static PyObject *Probe_reset_buffers (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_probe_reset_buffers (self->probe);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to reset buffers");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_reset_probe__doc__,
"reset_probe ()\n\n"
"Zeroize probing results and resets the current probing (this has impact to do_probe() only).\n"
"This function does not touch probing filters and keeps assigned device.");
static PyObject *Probe_reset_probe (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    blkid_reset_probe (self->probe);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_hide_range__doc__,
"hide_range (offset, length)\n\n" \
"This function modifies in-memory cached data from the device. The specified range is zeroized. "
"This is usable together with Probe.step_back(). The next Probe.do_probe() will not see specified area.\n"
"Note that this is usable for already (by library) read data, and this function is not a way "
"how to hide any large areas on your device.\n"
"The function Probe.reset_buffers() reverts all.");
static PyObject *Probe_hide_range (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    char *kwlist[] = { "offset", "length", NULL };
    uint64_t offset = 0;
    uint64_t length = 0;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ii", kwlist, &offset, &length)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_probe_hide_range (self->probe, offset, length);
    if (ret != 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to hide range");
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Probe_do_wipe__doc__,
"do_wipe (dryrun=False)\n\n"
"This function erases the current signature detected by the probe. The probe has to be open in "
"O_RDWR mode, blkid.SUBLKS_MAGIC or/and blkid.PARTS_MAGIC flags has to be enabled (if you want "
"to erase also superblock with broken check sums then use blkid.SUBLKS_BADCSUM too).\n\n"
"After successful signature removing the probe prober will be moved one step back and the next "
"do_probe() call will again call previously called probing function. All in-memory cached data "
"from the device are always reset.");
static PyObject *Probe_do_wipe (ProbeObject *self, PyObject *args, PyObject *kwargs) {
    int ret = 0;
    char *kwlist[] = { "dryrun", NULL };
    bool dryrun = false;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|p", kwlist, &dryrun)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    ret = blkid_do_wipe (self->probe, dryrun);
    if (ret != 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to wipe the device: %s", strerror (errno));
        return NULL;
    }

    Py_RETURN_NONE;
}

static PyMethodDef Probe_methods[] = {
    {"set_device", (PyCFunction)(void(*)(void)) Probe_set_device, METH_VARARGS|METH_KEYWORDS, Probe_set_device__doc__},
    {"do_safeprobe", (PyCFunction) Probe_do_safeprobe, METH_NOARGS, Probe_do_safeprobe__doc__},
    {"do_fullprobe", (PyCFunction) Probe_do_fullprobe, METH_NOARGS, Probe_do_fullprobe__doc__},
    {"do_probe", (PyCFunction) Probe_do_probe, METH_NOARGS, Probe_do_probe__doc__},
    {"step_back", (PyCFunction) Probe_step_back, METH_NOARGS, Probe_step_back__doc__},
    {"reset_buffers", (PyCFunction) Probe_reset_buffers, METH_NOARGS, Probe_reset_buffers__doc__},
    {"reset_probe", (PyCFunction) Probe_reset_probe, METH_NOARGS, Probe_reset_probe__doc__},
    {"hide_range", (PyCFunction)(void(*)(void)) Probe_hide_range, METH_VARARGS|METH_KEYWORDS, Probe_hide_range__doc__},
    {"do_wipe", (PyCFunction)(void(*)(void)) Probe_do_wipe, METH_VARARGS|METH_KEYWORDS, Probe_do_wipe__doc__},
    {"enable_partitions", (PyCFunction)(void(*)(void)) Probe_enable_partitions, METH_VARARGS|METH_KEYWORDS, Probe_enable_partitions__doc__},
    {"set_partitions_flags", (PyCFunction)(void(*)(void)) Probe_set_partitions_flags, METH_VARARGS|METH_KEYWORDS, Probe_set_partitions_flags__doc__},
    {"enable_superblocks", (PyCFunction)(void(*)(void)) Probe_enable_superblocks, METH_VARARGS|METH_KEYWORDS, Probe_enable_superblocks__doc__},
    {"filter_superblocks_type", (PyCFunction)(void(*)(void)) Probe_filter_superblocks_type, METH_VARARGS|METH_KEYWORDS, Probe_filter_superblocks_type__doc__},
    {"filter_superblocks_usage", (PyCFunction)(void(*)(void)) Probe_filter_superblocks_usage, METH_VARARGS|METH_KEYWORDS, Probe_filter_superblocks_usage__doc__},
    {"set_superblocks_flags", (PyCFunction)(void(*)(void)) Probe_set_superblocks_flags, METH_VARARGS|METH_KEYWORDS, Probe_set_superblocks_flags__doc__},
    {"invert_superblocks_filter", (PyCFunction) Probe_invert_superblocks_filter, METH_NOARGS, Probe_invert_superblocks_filter__doc__},
    {"reset_superblocks_filter", (PyCFunction) Probe_reset_superblocks_filter, METH_NOARGS, Probe_reset_superblocks_filter__doc__},
    {"lookup_value", (PyCFunction)(void(*)(void)) Probe_lookup_value, METH_VARARGS|METH_KEYWORDS, Probe_lookup_value__doc__},
};

static PyObject *Probe_get_devno (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    dev_t devno = blkid_probe_get_devno (self->probe);

    return PyLong_FromUnsignedLong (devno);
}

static PyObject *Probe_get_fd (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    return PyLong_FromLong (self->fd);
}

static PyObject *Probe_get_offset (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
	blkid_loff_t offset = blkid_probe_get_offset (self->probe);

    return PyLong_FromLongLong (offset);
}

static PyObject *Probe_get_sectors (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
	blkid_loff_t sectors = blkid_probe_get_sectors (self->probe);

    return PyLong_FromLongLong (sectors);
}

static PyObject *Probe_get_size (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
	blkid_loff_t size = blkid_probe_get_size (self->probe);

    return PyLong_FromLongLong (size);
}

static PyObject *Probe_get_sector_size (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
	unsigned int sector_size = blkid_probe_get_sectorsize (self->probe);

    return PyLong_FromUnsignedLong (sector_size);
}

static int Probe_set_sector_size (ProbeObject *self, PyObject *value, void *closure UNUSED) {
	unsigned int sector_size = 0;
    int ret = 0;

    if (!PyLong_Check (value)) {
		PyErr_SetString (PyExc_TypeError, "Invalid argument");

        return -1;
	}

    sector_size = PyLong_AsLong (value);

    ret = blkid_probe_set_sectorsize (self->probe, sector_size);
    if (ret != 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to set sector size");
        return -1;
    }

    return 0;
}

static PyObject *Probe_get_wholedisk_devno (ProbeObject *self, PyObject *Py_UNUSED (ignored)) {
    dev_t devno = blkid_probe_get_wholedisk_devno (self->probe);

    return PyLong_FromUnsignedLong (devno);
}

static PyObject *Probe_get_is_wholedisk (ProbeObject *self __attribute__((unused)), PyObject *Py_UNUSED (ignored)) {
    int wholedisk = blkid_probe_is_wholedisk (self->probe);

    return PyBool_FromLong (wholedisk);
}

static PyGetSetDef Probe_getseters[] = {
    {"devno", (getter) Probe_get_devno, NULL, "block device number, or 0 for regular files", NULL},
    {"fd", (getter) Probe_get_fd, NULL, "file descriptor for assigned device/file or -1 in case of error", NULL},
    {"offset", (getter) Probe_get_offset, NULL, "offset of probing area as defined by Probe.set_device() or -1 in case of error", NULL},
    {"sectors", (getter) Probe_get_sectors, NULL, "512-byte sector count or -1 in case of error", NULL},
    {"size", (getter) Probe_get_size, NULL, "size of probing area as defined by Probe.set_device()", NULL},
    {"sector_size", (getter) Probe_get_sector_size, (setter) Probe_set_sector_size, "block device logical sector size (BLKSSZGET ioctl, default 512).", NULL},
    {"wholedisk_devno", (getter) Probe_get_wholedisk_devno, NULL, "device number of the wholedisk, or 0 for regular files", NULL},
    {"is_wholedisk", (getter) Probe_get_is_wholedisk, NULL, "True if the device is whole-disk, False otherwise", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

PyTypeObject ProbeType = {
    PyVarObject_HEAD_INIT (NULL, 0)
    .tp_name = "blkid.Probe",
    .tp_basicsize = sizeof (ProbeObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Probe_new,
    .tp_dealloc = (destructor) Probe_dealloc,
    .tp_init = (initproc) Probe_init,
    .tp_methods = Probe_methods,
    .tp_getset = Probe_getseters,
};
