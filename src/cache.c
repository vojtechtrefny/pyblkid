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

static PyMethodDef Cache_methods[] = {
    {"probe_all", (PyCFunction)(void(*)(void)) Cache_probe_all, METH_VARARGS|METH_KEYWORDS, Cache_probe_all__doc__},
    {"gc", (PyCFunction) Cache_gc, METH_NOARGS, Cache_gc__doc__},
    {NULL, NULL, 0, NULL},
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
};
