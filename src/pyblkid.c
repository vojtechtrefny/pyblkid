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

#include <blkid/blkid.h>

#define UNUSED __attribute__((unused))


PyDoc_STRVAR(Blkid_init_debug__doc__,
"init_debug (mask)\n\n"
"If the mask is not specified then this function reads LIBBLKID_DEBUG environment variable to get the mask.\n"
"Already initialized debugging stuff cannot be changed. It does not have effect to call this function twice.\n\n"
"Use '0xffff' to enable full debugging.\n");
static PyObject *Blkid_init_debug (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    int mask = 0;
    char *kwlist[] = { "mask", NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "|i", kwlist, &mask)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    blkid_init_debug (mask);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Blkid_known_fstype__doc__,
"known_fstype (fstype)\n\n"
"Returns whether fstype is a known filesystem type or not.\n");
static PyObject *Blkid_known_fstype (PyObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    const char *fstype = NULL;
    char *kwlist[] = { "fstype", NULL };

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s", kwlist, &fstype)) {
        PyErr_SetString (PyExc_AttributeError, "Failed to parse arguments");
        return NULL;
    }

    return PyBool_FromLong (blkid_known_fstype (fstype));
}

static PyMethodDef BlkidMethods[] = {
    {"init_debug", (PyCFunction)(void(*)(void)) Blkid_init_debug, METH_VARARGS|METH_KEYWORDS, Blkid_init_debug__doc__},
    {"known_fstype", (PyCFunction)(void(*)(void)) Blkid_known_fstype, METH_VARARGS|METH_KEYWORDS, Blkid_known_fstype__doc__},
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

    return module;
}
