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

PyDoc_STRVAR(Partlist_get_partition__doc__,
"get_partition (number)\n\n"
"Get partition by number.\n\n"
"It's possible that the list of partitions is *empty*, but there is a valid partition table on the disk.\n"
"This happen when on-disk details about partitions are unknown or the partition table is empty.");
static PyObject *Partlist_get_partition (PartlistObject *self, PyObject *args, PyObject *kwargs) {
    char *kwlist[] = { "number", NULL };
    int partnum = 0;
    int numof = 0;
    blkid_partition blkid_part = NULL;
    PartitionObject *result = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "i", kwlist, &partnum)) {
        return NULL;
    }

    numof = blkid_partlist_numof_partitions (self->partlist);
    if (numof < 0) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to get number of partitions");
        return NULL;
    }

    if (partnum > numof) {
        PyErr_Format (PyExc_RuntimeError, "Cannot get partition %d, partition table has only %d partitions", partnum, numof);
        return NULL;
    }

    blkid_part = blkid_partlist_get_partition (self->partlist, partnum);
    if (!blkid_part) {
        PyErr_Format (PyExc_RuntimeError, "Failed to get partition %d", partnum);
        return NULL;
    }

    result = PyObject_New (PartitionObject, &PartitionType);
    if (!result) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create a new Partition object");
        return NULL;
    }

    Py_INCREF (result);
    result->number = partnum;
    result->partition = blkid_part;
    result->Parttable_object = NULL;

    return (PyObject *) result;
}

PyDoc_STRVAR(Partlist_get_partition_by_partno__doc__,
"get_partition_by_partno(number)\n\n"
"Get partition by partition number.\n\n"
"This does not assume any order of partitions and correctly handles \"out of order\" "
"partition tables. partition N is located after partition N+1 on the disk.");
static PyObject *Partlist_get_partition_by_partno (PartlistObject *self, PyObject *args, PyObject *kwargs) {
    char *kwlist[] = { "number", NULL };
    int partno = 0;
    blkid_partition blkid_part = NULL;
    PartitionObject *result = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "i", kwlist, &partno)) {
        return NULL;
    }

    blkid_part = blkid_partlist_get_partition_by_partno (self->partlist, partno);
    if (!blkid_part) {
        PyErr_Format (PyExc_RuntimeError, "Failed to get partition %d", partno);
        return NULL;
    }

    result = PyObject_New (PartitionObject, &PartitionType);
    if (!result) {
        PyErr_NoMemory ();
        return NULL;
    }

    Py_INCREF (result);
    result->number = partno;
    result->partition = blkid_part;
    result->Parttable_object = NULL;

    return (PyObject *) result;
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

PyDoc_STRVAR(Partlist_devno_to_partition__doc__,
"devno_to_partition (devno)\n\n"
"Get partition by devno.\n");
static PyObject *Partlist_devno_to_partition (PartlistObject *self, PyObject *args, PyObject *kwargs) {
    dev_t devno = 0;
    char *kwlist[] = { "devno", NULL };
    blkid_partition blkid_part = NULL;
    PartitionObject *result = NULL;

    if (!PyArg_ParseTupleAndKeywords (args, kwargs, "O&:devno_to_devname", kwlist, _Py_Dev_Converter, &devno))
        return NULL;

    blkid_part = blkid_partlist_devno_to_partition (self->partlist, devno);
    if (!blkid_part) {
        PyErr_Format (PyExc_RuntimeError, "Failed to get partition %zu", devno);
        return NULL;
    }

    result = PyObject_New (PartitionObject, &PartitionType);
    if (!result) {
        PyErr_NoMemory ();
        return NULL;
    }

    Py_INCREF (result);
    result->number = blkid_partition_get_partno (blkid_part);
    result->partition = blkid_part;
    result->Parttable_object = NULL;

    return (PyObject *) result;
}

static PyMethodDef Partlist_methods[] = {
    {"get_partition", (PyCFunction)(void(*)(void)) Partlist_get_partition, METH_VARARGS|METH_KEYWORDS, Partlist_get_partition__doc__},
    {"get_partition_by_partno", (PyCFunction)(void(*)(void)) Partlist_get_partition_by_partno, METH_VARARGS|METH_KEYWORDS, Partlist_get_partition_by_partno__doc__},
    {"devno_to_partition", (PyCFunction)(void(*)(void)) Partlist_devno_to_partition, METH_VARARGS|METH_KEYWORDS, Partlist_devno_to_partition__doc__},
    {NULL, NULL, 0, NULL},
};

static PyObject *Partlist_get_table (PartlistObject *self, PyObject *Py_UNUSED (ignored)) {
    if (self->Parttable_object) {
        Py_INCREF (self->Parttable_object);
        return self->Parttable_object;
    }

    self->Parttable_object = _Parttable_get_parttable_object (self->partlist);

    return self->Parttable_object;
}

static PyObject *Partlist_get_numof_partitions (PartlistObject *self, PyObject *Py_UNUSED (ignored)) {
    int ret = 0;

    ret = blkid_partlist_numof_partitions (self->partlist);
    if (ret < 0) {
        PyErr_SetString (PyExc_MemoryError, "Failed to get number of partitions");
        return NULL;
    }

    return PyLong_FromLong (ret);
}


static PyGetSetDef Partlist_getseters[] = {
    {"table", (getter) Partlist_get_table, NULL, "binary interface for partition table on the device", NULL},
    {"numof_partitions", (getter) Partlist_get_numof_partitions, NULL, "number of partitions in the list", NULL},
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
    .tp_methods = Partlist_methods,
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

PyDoc_STRVAR(Parttable_get_parent__doc__,
"get_parent ()\n\n"
"Parent for nested partition tables.");
static PyObject *Parttable_get_parent (ParttableObject *self, PyObject *Py_UNUSED (ignored)) {
    blkid_partition blkid_part = NULL;
    PartitionObject *result = NULL;

    blkid_part = blkid_parttable_get_parent (self->table);
    if (!blkid_part)
        Py_RETURN_NONE;

    result = PyObject_New (PartitionObject, &PartitionType);
    if (!result) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create a new Partition object");
        return NULL;
    }

    Py_INCREF (result);
    result->number = 0;
    result->partition = blkid_part;

    return (PyObject *) result;
}

static PyMethodDef Parttable_methods[] = {
    {"get_parent", (PyCFunction)(void(*)(void)) Parttable_get_parent, METH_NOARGS, Parttable_get_parent__doc__},
    {NULL, NULL, 0, NULL},
};

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
    .tp_methods = Parttable_methods,
    .tp_getset = Parttable_getseters,
};

/*********************** PARTITION ***********************/
PyObject *Partition_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    PartitionObject *self = (PartitionObject*) type->tp_alloc (type, 0);

    if (self)
        self->Parttable_object = NULL;

    return (PyObject *) self;
}

int Partition_init (PartitionObject *self, PyObject *args, PyObject *kwargs) {
    char *kwlist[] = { "number", NULL };

     if (!PyArg_ParseTupleAndKeywords (args, kwargs, "i", kwlist, &(self->number))) {
        return -1;
    }

    self->partition = NULL;

    return 0;
}

void Partition_dealloc (PartitionObject *self) {
    if (self->Parttable_object)
        Py_DECREF (self->Parttable_object);

    Py_TYPE (self)->tp_free ((PyObject *) self);
}

static PyObject *Partition_get_type (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    int type = blkid_partition_get_type (self->partition);

    return PyLong_FromLong (type);
}

static PyObject *Partition_get_type_string (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    const char *type = blkid_partition_get_type_string (self->partition);

    return PyUnicode_FromString (type);
}

static PyObject *Partition_get_uuid (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    const char *uuid = blkid_partition_get_uuid (self->partition);

    return PyUnicode_FromString (uuid);
}

static PyObject *Partition_get_is_extended (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    int extended = blkid_partition_is_extended (self->partition);

    if (extended == 1)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *Partition_get_is_logical (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    int logical = blkid_partition_is_logical (self->partition);

    if (logical == 1)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *Partition_get_is_primary (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    int primary = blkid_partition_is_primary (self->partition);

    if (primary == 1)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}

static PyObject *Partition_get_name (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    const char *name = blkid_partition_get_name (self->partition);

    return PyUnicode_FromString (name);
}

static PyObject *Partition_get_flags (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    unsigned long long flags = blkid_partition_get_flags (self->partition);

    return PyLong_FromUnsignedLongLong (flags);
}

static PyObject *Partition_get_partno (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    int partno = blkid_partition_get_partno (self->partition);

    return PyLong_FromLong (partno);
}

static PyObject *Partition_get_size (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    blkid_loff_t size = blkid_partition_get_size (self->partition);

    return PyLong_FromLongLong (size);
}

static PyObject *Partition_get_start (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    blkid_loff_t start = blkid_partition_get_start (self->partition);

    return PyLong_FromLongLong (start);
}

PyObject *_Partition_get_parttable_object (blkid_partition partition) {
    ParttableObject *result = NULL;
    blkid_parttable table = NULL;

    if (!partition) {
        PyErr_SetString(PyExc_RuntimeError, "internal error");
        return NULL;
    }

    table = blkid_partition_get_table (partition);
    if (!table) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to get partition table");
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

static PyObject *Partition_get_table (PartitionObject *self, PyObject *Py_UNUSED (ignored)) {
    if (self->Parttable_object) {
        Py_INCREF (self->Parttable_object);
        return self->Parttable_object;
    }

    self->Parttable_object = _Partition_get_parttable_object (self->partition);

    return self->Parttable_object;
}

static PyGetSetDef Partition_getseters[] = {
    {"type", (getter) Partition_get_type, NULL, "partition type", NULL},
    {"type_string", (getter) Partition_get_type_string, NULL, "partition type string, note the type string is supported by a small subset of partition tables (e.g Mac and EFI GPT)", NULL},
    {"uuid", (getter) Partition_get_uuid, NULL, "partition UUID string if supported by PT (e.g. GPT)", NULL},
    {"is_extended", (getter) Partition_get_is_extended, NULL, "returns whether the partition is extendedor not ", NULL},
    {"is_logical", (getter) Partition_get_is_logical, NULL, "returns whether the partition is logical or not", NULL},
    {"is_primary", (getter) Partition_get_is_primary, NULL, "returns whether the partition is primary or not", NULL},
    {"name", (getter) Partition_get_name, NULL, "partition name string if supported by PT (e.g. Mac)", NULL},
    {"flags", (getter) Partition_get_flags, NULL, "partition flags (or attributes for gpt)", NULL},
    {"partno", (getter) Partition_get_partno, NULL, "proposed partition number (e.g. 'N' from sda'N') or -1 in case of error", NULL},
    {"size", (getter) Partition_get_size, NULL, "size of the partition (in 512-sectors)", NULL},
    {"start", (getter) Partition_get_start, NULL, "start of the partition (in 512-sectors)", NULL},
    {"table", (getter) Partition_get_table, NULL, "partition table object (usually the same for all partitions, except nested partition tables)", NULL},
    {NULL, NULL, NULL, NULL, NULL}
};

PyTypeObject PartitionType = {
    PyVarObject_HEAD_INIT (NULL, 0)
    .tp_name = "blkid.Partition",
    .tp_basicsize = sizeof (PartitionObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Partition_new,
    .tp_dealloc = (destructor) Partition_dealloc,
    .tp_init = (initproc) Partition_init,
    .tp_getset = Partition_getseters,
};
