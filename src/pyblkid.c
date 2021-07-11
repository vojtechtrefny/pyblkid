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

#include "pyblkid.h"
#include "probe.h"
#include "topology.h"
#include "partitions.h"
#include "cache.h"

#include <blkid/blkid.h>
#include <errno.h>
#include <fcntl.h>

#define UNUSED __attribute__((unused))


PyDoc_STRVAR(Blkid_init_debug__doc__,
"init_debug (mask)\n\n"
"If the mask is not specified then this function reads LIBBLKID_DEBUG environment variable to get the mask.\n"
"Already initialized debugging stuff cannot be changed. It does not have effect to call this function twice.\n\n"
"Use '0xffff' to enable full debugging.\n");
static PyObject *Blkid_init_debug (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    int mask = 0;
    char *kwlist[] = { "mask", NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "|i", kwlist, &mask))
        return NULL;

    blkid_init_debug (mask);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Blkid_known_fstype__doc__,
"known_fstype (fstype)\n\n"
"Returns whether fstype is a known filesystem type or not.\n");
static PyObject *Blkid_known_fstype (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    const char *fstype = NULL;
    char *kwlist[] = { "fstype", NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s", kwlist, &fstype))
        return NULL;

    return PyBool_FromLong (blkid_known_fstype (fstype));
}

PyDoc_STRVAR(Blkid_send_uevent__doc__,
"send_uevent (devname, action)\n\n");
static PyObject *Blkid_send_uevent (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    const char *devname = NULL;
    const char *action = NULL;
    char *kwlist[] = { "devname", "action", NULL };
    int ret = 0;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "ss", kwlist, &devname, &action))
        return NULL;

    ret = blkid_send_uevent (devname, action);
    if (ret < 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to send %s uevent do device '%s'", action, devname);
        return NULL;
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Blkid_known_pttype__doc__,
"known_pttype (pttype)\n\n"
"Returns whether pttype is a known partition type or not.\n");
static PyObject *Blkid_known_pttype (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    const char *pttype = NULL;
    char *kwlist[] = { "pttype", NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s", kwlist, &pttype))
        return NULL;

    return PyBool_FromLong (blkid_known_pttype (pttype));
}

static int _Py_Dev_Converter (PyObject *obj, void *p) {
#ifdef HAVE_LONG_LONG
    *((dev_t *)p) = PyLong_AsUnsignedLongLong (obj);
#else
    *((dev_t *)p) = PyLong_AsUnsignedLong (obj);
#endif
    if (PyErr_Occurred ())
        return 0;
    return 1;
}

#ifdef HAVE_LONG_LONG
    #define _PyLong_FromDev PyLong_FromLongLong
#else
    #define _PyLong_FromDev PyLong_FromLong
#endif

PyDoc_STRVAR(Blkid_devno_to_devname__doc__,
"devno_to_devname (devno)\n\n"
"This function finds the pathname to a block device with a given device number.\n");
static PyObject *Blkid_devno_to_devname (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    dev_t devno = 0;
    char *kwlist[] = { "devno", NULL };
    char *devname = NULL;
    PyObject *ret = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O&:devno_to_devname", kwlist, _Py_Dev_Converter, &devno))
        return NULL;

    devname = blkid_devno_to_devname (devno);
    if (!devname) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to get devname");
        return NULL;
    }

    ret = PyUnicode_FromString (devname);
    free (devname);

    return ret;
}

PyDoc_STRVAR(Blkid_parse_version_string__doc__,
"parse_version_string (version)\n\n"
"Convert version string (e.g. '2.16.0') to release version code (e.g. '2160').\n");
static PyObject *Blkid_parse_version_string (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    char *ver_str = NULL;
    char *kwlist[] = { "version", NULL };
    int ret = 0;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s", kwlist, &ver_str))
        return NULL;

    ret = blkid_parse_version_string (ver_str);

    return PyLong_FromLong (ret);
}

PyDoc_STRVAR(Blkid_get_library_version__doc__,
"get_library_version ()\n\n"
"Returns tuple of release version code (int), version string and date.\n");
static PyObject *Blkid_get_library_version (ProbeObject *self UNUSED, PyObject *Py_UNUSED (ignored)) {
    const char *ver_str = NULL;
    const char *date = NULL;
    int ver_code = 0;
    PyObject *ret = NULL;
    PyObject *py_code = NULL;
    PyObject *py_ver = NULL;
	PyObject *py_date = NULL;

    ver_code = blkid_get_library_version (&ver_str, &date);

    ret = PyTuple_New (3);

    py_code = PyLong_FromLong (ver_code);
    PyTuple_SetItem (ret, 0, py_code);

    py_ver = PyUnicode_FromString (ver_str);
    if (py_ver == NULL) {
        Py_INCREF (Py_None);
        py_ver = Py_None;
    }
    PyTuple_SetItem (ret, 1, py_ver);

    py_date = PyUnicode_FromString (date);
    if (py_date == NULL) {
        Py_INCREF (Py_None);
        py_date = Py_None;
    }
    PyTuple_SetItem (ret, 2, py_date);

    return ret;
}

PyDoc_STRVAR(Blkid_parse_tag_string__doc__,
"parse_tag_string (tag)\n\n"
"Parse a 'NAME=value' string, returns tuple of type and value.\n");
static PyObject *Blkid_parse_tag_string (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    char *tag_str = NULL;
    char *kwlist[] = { "tag", NULL };
    int ret = 0;
    char *type = NULL;
    char *value = NULL;
    PyObject *py_type = NULL;
	PyObject *py_value = NULL;
    PyObject *tuple = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s", kwlist, &tag_str))
        return NULL;

    ret = blkid_parse_tag_string (tag_str, &type, &value);
    if (ret < 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to parse tag '%s'", tag_str);
        return NULL;
    }

    tuple = PyTuple_New (2);

    py_type = PyUnicode_FromString (type);
    if (py_type == NULL) {
        Py_INCREF (Py_None);
        py_type = Py_None;
    }
    PyTuple_SetItem (tuple, 0, py_type);

    py_value = PyUnicode_FromString (value);
    if (py_value == NULL) {
        Py_INCREF (Py_None);
        py_value = Py_None;
    }
    PyTuple_SetItem (tuple, 1, py_value);

    return tuple;
}

PyDoc_STRVAR(Blkid_get_dev_size__doc__,
"get_dev_size (device)\n\n"
"Returns size (in bytes) of the block device or size of the regular file.\n");
static PyObject *Blkid_get_dev_size (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    char *device = NULL;
    char *kwlist[] = { "device", NULL };
    blkid_loff_t ret = 0;
    int fd = 0;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s", kwlist, &device))
        return NULL;

    fd = open (device, O_RDONLY|O_CLOEXEC);
    if (fd == -1) {
        PyErr_Format (PyExc_OSError, "Failed to open device '%s': %s", device, strerror (errno));
        return NULL;
    }

    ret = blkid_get_dev_size (fd);
    if (ret == 0) {
        PyErr_Format (PyExc_RuntimeError, "Failed to get size of device '%s': %s", device);
        close (fd);
        return NULL;
    }

    close (fd);
    return PyLong_FromLongLong (ret);
}

static PyMethodDef BlkidMethods[] = {
    {"init_debug", (PyCFunction)(void(*)(void)) Blkid_init_debug, METH_VARARGS|METH_KEYWORDS, Blkid_init_debug__doc__},
    {"known_fstype", (PyCFunction)(void(*)(void)) Blkid_known_fstype, METH_VARARGS|METH_KEYWORDS, Blkid_known_fstype__doc__},
    {"send_uevent", (PyCFunction)(void(*)(void)) Blkid_send_uevent, METH_VARARGS|METH_KEYWORDS, Blkid_send_uevent__doc__},
    {"devno_to_devname", (PyCFunction)(void(*)(void)) Blkid_devno_to_devname, METH_VARARGS|METH_KEYWORDS, Blkid_devno_to_devname__doc__},
    {"known_pttype", (PyCFunction)(void(*)(void)) Blkid_known_pttype, METH_VARARGS|METH_KEYWORDS, Blkid_known_pttype__doc__},
    {"parse_version_string", (PyCFunction)(void(*)(void)) Blkid_parse_version_string, METH_VARARGS|METH_KEYWORDS, Blkid_parse_version_string__doc__},
    {"get_library_version", (PyCFunction) Blkid_get_library_version, METH_NOARGS, Blkid_get_library_version__doc__},
    {"parse_tag_string", (PyCFunction)(void(*)(void)) Blkid_parse_tag_string, METH_VARARGS|METH_KEYWORDS, Blkid_parse_tag_string__doc__},
    {"get_dev_size", (PyCFunction)(void(*)(void)) Blkid_get_dev_size, METH_VARARGS|METH_KEYWORDS, Blkid_get_dev_size__doc__},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef blkidmodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "blkid",
    .m_doc = "Python interface for the libblkid C library",
    .m_size = -1,
    .m_methods = BlkidMethods,
};

PyMODINIT_FUNC PyInit_blkid (void) {
    PyObject *module = NULL;

    if (PyType_Ready (&ProbeType) < 0)
        return NULL;

    if (PyType_Ready (&TopologyType) < 0)
        return NULL;

    if (PyType_Ready (&PartlistType) < 0)
        return NULL;

    if (PyType_Ready (&ParttableType) < 0)
        return NULL;

    if (PyType_Ready (&PartitionType) < 0)
        return NULL;

    if (PyType_Ready (&CacheType) < 0)
        return NULL;

    if (PyType_Ready (&DeviceType) < 0)
        return NULL;

    module = PyModule_Create (&blkidmodule);
    if (!module)
        return NULL;

    PyModule_AddIntConstant (module, "FLTR_NOTIN", BLKID_FLTR_NOTIN);
    PyModule_AddIntConstant (module, "FLTR_ONLYIN", BLKID_FLTR_ONLYIN);

    PyModule_AddIntConstant (module, "DEV_CREATE", BLKID_DEV_CREATE);
    PyModule_AddIntConstant (module, "DEV_FIND", BLKID_DEV_FIND);
    PyModule_AddIntConstant (module, "DEV_NORMAL", BLKID_DEV_NORMAL);
    PyModule_AddIntConstant (module, "DEV_VERIFY", BLKID_DEV_VERIFY);

    PyModule_AddIntConstant (module, "PARTS_ENTRY_DETAILS", BLKID_PARTS_ENTRY_DETAILS);
    PyModule_AddIntConstant (module, "PARTS_FORCE_GPT", BLKID_PARTS_FORCE_GPT);
    PyModule_AddIntConstant (module, "PARTS_MAGIC", BLKID_PARTS_MAGIC);

    PyModule_AddIntConstant (module, "SUBLKS_BADCSUM", BLKID_SUBLKS_BADCSUM);
    PyModule_AddIntConstant (module, "SUBLKS_DEFAULT", BLKID_SUBLKS_DEFAULT);
    PyModule_AddIntConstant (module, "SUBLKS_LABEL", BLKID_SUBLKS_LABEL);
    PyModule_AddIntConstant (module, "SUBLKS_LABELRAW", BLKID_SUBLKS_LABELRAW);
    PyModule_AddIntConstant (module, "SUBLKS_MAGIC", BLKID_SUBLKS_MAGIC);
    PyModule_AddIntConstant (module, "SUBLKS_SECTYPE", BLKID_SUBLKS_SECTYPE);
    PyModule_AddIntConstant (module, "SUBLKS_TYPE", BLKID_SUBLKS_TYPE);
    PyModule_AddIntConstant (module, "SUBLKS_USAGE", BLKID_SUBLKS_USAGE);
    PyModule_AddIntConstant (module, "SUBLKS_UUID", BLKID_SUBLKS_UUID);
    PyModule_AddIntConstant (module, "SUBLKS_UUIDRAW", BLKID_SUBLKS_UUIDRAW);
    PyModule_AddIntConstant (module, "SUBLKS_VERSION", BLKID_SUBLKS_VERSION);

    PyModule_AddIntConstant (module, "USAGE_CRYPTO", BLKID_USAGE_CRYPTO);
    PyModule_AddIntConstant (module, "USAGE_FILESYSTEM", BLKID_USAGE_FILESYSTEM);
    PyModule_AddIntConstant (module, "USAGE_OTHER", BLKID_USAGE_OTHER);
    PyModule_AddIntConstant (module, "USAGE_RAID", BLKID_USAGE_RAID);

    Py_INCREF (&ProbeType);
    if (PyModule_AddObject (module, "Probe", (PyObject *) &ProbeType) < 0) {
        Py_DECREF (&ProbeType);
        Py_DECREF (module);
        return NULL;
    }

    Py_INCREF (&TopologyType);
    if (PyModule_AddObject (module, "Topology", (PyObject *) &TopologyType) < 0) {
        Py_DECREF (&ProbeType);
        Py_DECREF (&TopologyType);
        Py_DECREF (module);
        return NULL;
    }

    Py_INCREF (&PartlistType);
    if (PyModule_AddObject (module, "Partlist", (PyObject *) &PartlistType) < 0) {
        Py_DECREF (&ProbeType);
        Py_DECREF (&TopologyType);
        Py_DECREF (&PartlistType);
        Py_DECREF (module);
        return NULL;
    }

    Py_INCREF (&ParttableType);
    if (PyModule_AddObject (module, "Parttable", (PyObject *) &ParttableType) < 0) {
        Py_DECREF (&ProbeType);
        Py_DECREF (&TopologyType);
        Py_DECREF (&PartlistType);
        Py_DECREF (&ParttableType);
        Py_DECREF (module);
        return NULL;
    }

    Py_INCREF (&PartitionType);
    if (PyModule_AddObject (module, "Partition", (PyObject *) &PartitionType) < 0) {
        Py_DECREF (&ProbeType);
        Py_DECREF (&TopologyType);
        Py_DECREF (&PartlistType);
        Py_DECREF (&ParttableType);
        Py_DECREF (&PartitionType);
        Py_DECREF (module);
        return NULL;
    }

    Py_INCREF (&CacheType);
    if (PyModule_AddObject (module, "Cache", (PyObject *) &CacheType) < 0) {
        Py_DECREF (&ProbeType);
        Py_DECREF (&TopologyType);
        Py_DECREF (&PartlistType);
        Py_DECREF (&ParttableType);
        Py_DECREF (&PartitionType);
        Py_DECREF (&CacheType);
        Py_DECREF (module);
        return NULL;
    }

    Py_INCREF (&DeviceType);
    if (PyModule_AddObject (module, "Device", (PyObject *) &DeviceType) < 0) {
        Py_DECREF (&ProbeType);
        Py_DECREF (&TopologyType);
        Py_DECREF (&PartlistType);
        Py_DECREF (&ParttableType);
        Py_DECREF (&PartitionType);
        Py_DECREF (&CacheType);
        Py_DECREF (&DeviceType);
        Py_DECREF (module);
        return NULL;
    }

    return module;
}
