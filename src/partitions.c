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

#include "partitions.h"

#include <blkid/blkid.h>

#define UNUSED __attribute__((unused))


/*********************** PARTLIST ***********************/
PyObject *Partlist_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    PartlistObject *self = (PartlistObject*) type->tp_alloc (type, 0);

    if (self)
        self->Parttable_object = NULL;

    return (PyObject *) self;
}

int Partlist_init (PartlistObject *self UNUSED, PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    return 0;
}

void Partlist_dealloc (PartlistObject *self) {
    if (self->Parttable_object)
        Py_DECREF (self->Parttable_object);

    Py_TYPE (self)->tp_free ((PyObject *) self);
}

PyObject *_Partlist_get_partlist_object (blkid_probe probe) {
    PartlistObject *result = NULL;
    blkid_partlist partlist = NULL;

    if (!probe) {
        PyErr_SetString (PyExc_RuntimeError, "internal error");
        return NULL;
    }

    partlist = blkid_probe_get_partitions (probe);
    if (!partlist) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to get partitions");
        return NULL;
    }

    result = PyObject_New (PartlistObject, &PartlistType);
    if (!result) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create a new Partlist object");
        return NULL;
    }
    Py_INCREF (result);

    result->partlist = partlist;
    result->Parttable_object = NULL;

    return (PyObject *) result;
}

static PyObject *Partlist_get_table (PartlistObject *self, PyObject *Py_UNUSED (ignored)) {
    if (self->Parttable_object) {
        Py_INCREF (self->Parttable_object);
        return self->Parttable_object;
    }

    self->Parttable_object = _Parttable_get_parttable_object (self->partlist);

    return self->Parttable_object;
}

static PyGetSetDef Partlist_getseters[] = {
    {"table", (getter) Partlist_get_table, NULL, "binary interface for partition table on the device", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

PyTypeObject PartlistType = {
    PyVarObject_HEAD_INIT (NULL, 0)
    .tp_name = "blkid.Partlist",
    .tp_basicsize = sizeof (PartlistObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Partlist_new,
    .tp_dealloc = (destructor) Partlist_dealloc,
    .tp_init = (initproc) Partlist_init,
    .tp_getset = Partlist_getseters,
};


/*********************** PARTTABLE ***********************/
PyObject *Parttable_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    ParttableObject *self = (ParttableObject*) type->tp_alloc (type, 0);

    return (PyObject *) self;
}

int Parttable_init (ParttableObject *self UNUSED, PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    return 0;
}

void Parttable_dealloc (ParttableObject *self) {
    Py_TYPE (self)->tp_free ((PyObject *) self);
}

PyObject *_Parttable_get_parttable_object (blkid_partlist partlist) {
    ParttableObject *result = NULL;
    blkid_parttable table = NULL;

    if (!partlist) {
        PyErr_SetString(PyExc_RuntimeError, "internal error");
        return NULL;
    }

    table = blkid_partlist_get_table (partlist);
    if (!table) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to get partitions");
        return NULL;
    }

    result = PyObject_New (ParttableObject, &ParttableType);
    if (!result) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create a new Parttable object");
        return NULL;
    }
    Py_INCREF (result);

    result->table = table;

    return (PyObject *) result;
}

static PyObject *Parrtable_get_type (ParttableObject *self, PyObject *Py_UNUSED (ignored)) {
    const char *pttype = blkid_parttable_get_type (self->table);

    return PyUnicode_FromString (pttype);
}

static PyObject *Parrtable_get_id (ParttableObject *self, PyObject *Py_UNUSED (ignored)) {
    const char *ptid = blkid_parttable_get_id (self->table);

    return PyUnicode_FromString (ptid);
}

static PyObject *Parrtable_get_offset (ParttableObject *self, PyObject *Py_UNUSED (ignored)) {
    blkid_loff_t offset = blkid_parttable_get_offset (self->table);

    return PyLong_FromLongLong (offset);
}

static PyGetSetDef Parttable_getseters[] = {
    {"type", (getter) Parrtable_get_type, NULL, "partition table type (type name, e.g. 'dos', 'gpt', ...)", NULL},
    {"id", (getter) Parrtable_get_id, NULL, "GPT disk UUID or DOS disk ID (in hex format)", NULL},
    {"offset", (getter) Parrtable_get_offset, NULL, "position (in bytes) of the partition table", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

PyTypeObject ParttableType = {
    PyVarObject_HEAD_INIT (NULL, 0)
    .tp_name = "blkid.Parttable",
    .tp_basicsize = sizeof (ParttableObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Parttable_new,
    .tp_dealloc = (destructor) Parttable_dealloc,
    .tp_init = (initproc) Parttable_init,
    .tp_getset = Parttable_getseters,
};