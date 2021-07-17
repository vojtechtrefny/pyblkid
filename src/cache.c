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

#include "cache.h"

#include <blkid/blkid.h>
#include <stdbool.h>

#define UNUSED __attribute__((unused))


PyObject *Cache_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    CacheObject *self = (CacheObject*) type->tp_alloc (type, 0);

    if (self)
        self->cache = NULL;

    return (PyObject *) self;
}

int Cache_init (CacheObject *self UNUSED, PyObject *args, PyObject *kwargs) {
    char *filename = NULL;
    char *kwlist[] = { "filename", NULL };
    int ret = 0;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "|s", kwlist, &filename)) {
        return -1;
    }

    ret = blkid_get_cache (&(self->cache), filename);
    if (ret < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to get cache");
        return -1;
    }

    return 0;
}

void Cache_dealloc (CacheObject *self) {
    Py_TYPE (self)->tp_free ((PyObject *) self);
}

PyDoc_STRVAR(Cache_probe_all__doc__,
"probe_all (removable=False)\n\n"
"Probes all block devices.\n\n"
"With removable=True also adds removable block devices to cache. Don't forget that "
"removable devices could be pretty slow. It's very bad idea to call this function by default.");
static PyObject *Cache_probe_all (CacheObject *self, PyObject *args, PyObject *kwargs) {
    bool removable = false;
    char *kwlist[] = { "removable", NULL };
    int ret = 0;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "|p", kwlist, &removable)) {
        return NULL;
    }

    ret = blkid_probe_all (self->cache);
    if (ret < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to probe block devices");
        return NULL;
    }

    if (removable) {
        ret = blkid_probe_all_removable (self->cache);
        if (ret < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to probe removable devices");
        return NULL;
        }
    }

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Cache_gc__doc__,
"gc\n\n"
"Removes garbage (non-existing devices) from the cache.");
static PyObject *Cache_gc (CacheObject *self, PyObject *Py_UNUSED (ignored))  {
    blkid_gc_cache (self->cache);

    Py_RETURN_NONE;
}

PyDoc_STRVAR(Cache_get_device__doc__,
"get_device (name)\n\n"
"Get device from cache.\n\n");
static PyObject *Cache_get_device (CacheObject *self, PyObject *args, PyObject *kwargs) {
    const char *name = NULL;
    char *kwlist[] = { "name", NULL };
    blkid_dev device = NULL;
    DeviceObject *dev_obj = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "s", kwlist, &name))
        return NULL;

    device = blkid_get_dev (self->cache, name, BLKID_DEV_FIND);
    if (device == NULL)
        Py_RETURN_NONE;

    dev_obj = PyObject_New (DeviceObject, &DeviceType);
    if (!dev_obj) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create a new Device object");
        return NULL;
    }

    dev_obj->device = device;

    Py_INCREF (dev_obj);
    return (PyObject *)  dev_obj;
}

PyDoc_STRVAR(Cache_find_device__doc__,
"find_device (tag, value)\n\n"
"Returns a device which matches a particular tag/value pair.\n"
" If there is more than one device that matches the search specification, "
"it returns the one with the highest priority\n\n");
static PyObject *Cache_find_device (CacheObject *self, PyObject *args, PyObject *kwargs) {
    const char *tag = NULL;
    const char *value = NULL;
    char *kwlist[] = { "tag", "value", NULL };
    blkid_dev device = NULL;
    DeviceObject *dev_obj = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "ss", kwlist, &tag, &value))
        return NULL;

    device = blkid_find_dev_with_tag (self->cache, tag, value);
    if (device == NULL)
        Py_RETURN_NONE;

    dev_obj = PyObject_New (DeviceObject, &DeviceType);
    if (!dev_obj) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create a new Device object");
        return NULL;
    }

    dev_obj->device = device;

    Py_INCREF (dev_obj);
    return (PyObject *)  dev_obj;
}

static PyMethodDef Cache_methods[] = {
    {"probe_all", (PyCFunction)(void(*)(void)) Cache_probe_all, METH_VARARGS|METH_KEYWORDS, Cache_probe_all__doc__},
    {"gc", (PyCFunction) Cache_gc, METH_NOARGS, Cache_gc__doc__},
    {"get_device", (PyCFunction)(void(*)(void)) Cache_get_device, METH_VARARGS|METH_KEYWORDS, Cache_get_device__doc__},
    {"find_device", (PyCFunction)(void(*)(void)) Cache_find_device, METH_VARARGS|METH_KEYWORDS, Cache_find_device__doc__},
    {NULL, NULL, 0, NULL},
};

static PyObject *Cache_get_devices (CacheObject *self, PyObject *Py_UNUSED (ignored)) {
    blkid_dev_iterate iter;
    blkid_dev device = NULL;
    DeviceObject *dev_obj = NULL;
    PyObject *list = NULL;

    list = PyList_New (0);
    if (!list) {
        PyErr_NoMemory ();
        return NULL;
    }

    iter = blkid_dev_iterate_begin (self->cache);
    while (blkid_dev_next (iter, &device) == 0) {
        dev_obj = PyObject_New (DeviceObject, &DeviceType);
        if (!dev_obj) {
            PyErr_NoMemory ();
            return NULL;
        }
        dev_obj->device = device;
        PyList_Append (list, (PyObject *) dev_obj);

	}
	blkid_dev_iterate_end(iter);

    return (PyObject *) list;
}

static PyGetSetDef Cache_getseters[] = {
    {"devices", (getter) Cache_get_devices, NULL, "returns all devices in the cache", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

PyTypeObject CacheType = {
    PyVarObject_HEAD_INIT (NULL, 0)
    .tp_name = "blkid.Cache",
    .tp_basicsize = sizeof (CacheObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Cache_new,
    .tp_dealloc = (destructor) Cache_dealloc,
    .tp_init = (initproc) Cache_init,
    .tp_methods = Cache_methods,
    .tp_getset = Cache_getseters,
};

/*********************** DEVICE ***********************/
PyObject *Device_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    DeviceObject *self = (DeviceObject*) type->tp_alloc (type, 0);

    if (self)
        self->device = NULL;

    return (PyObject *) self;
}

int Device_init (DeviceObject *self UNUSED, PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    return 0;
}

void Device_dealloc (DeviceObject *self) {
    Py_TYPE (self)->tp_free ((PyObject *) self);
}

static PyObject *Device_get_devname (DeviceObject *self, PyObject *Py_UNUSED (ignored)) {
    const char *name = blkid_dev_devname (self->device);

    if (!name)
        Py_RETURN_NONE;

    return PyUnicode_FromString (name);
}

static PyObject *Device_get_tags (DeviceObject *self, PyObject *Py_UNUSED (ignored)) {
    blkid_tag_iterate iter;
	const char *type = NULL;
    const char *value = NULL;
    PyObject *dict = NULL;
    PyObject *py_value = NULL;

    dict = PyDict_New ();
    if (!dict) {
        PyErr_NoMemory ();
        return NULL;
    }

    iter = blkid_tag_iterate_begin (self->device);
    while (blkid_tag_next (iter, &type, &value) == 0) {
        py_value = PyUnicode_FromString (value);
        if (py_value == NULL) {
            Py_INCREF (Py_None);
            py_value = Py_None;
        }

        PyDict_SetItemString (dict, type, py_value);
    }
    blkid_tag_iterate_end(iter);

    return (PyObject *) dict;
}

static PyGetSetDef Device_getseters[] = {
    {"devname", (getter) Device_get_devname, NULL, "returns the name previously used for Cache.get_device.", NULL},
    {"tags", (getter) Device_get_tags, NULL, "returns all tags for this device.", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

PyTypeObject DeviceType = {
    PyVarObject_HEAD_INIT (NULL, 0)
    .tp_name = "blkid.Device",
    .tp_basicsize = sizeof (DeviceObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Device_new,
    .tp_dealloc = (destructor) Device_dealloc,
    .tp_init = (initproc) Device_init,
    .tp_getset = Device_getseters,
};
