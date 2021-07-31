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

#include "topology.h"

#include <blkid/blkid.h>

#define UNUSED __attribute__((unused))


PyObject *Topology_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    TopologyObject *self = (TopologyObject*) type->tp_alloc (type, 0);

    return (PyObject *) self;
}

int Topology_init (TopologyObject *self UNUSED, PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    return 0;
}

void Topology_dealloc (TopologyObject *self) {
    Py_TYPE (self)->tp_free ((PyObject *) self);
}

PyObject *_Topology_get_topology_object (blkid_probe probe) {
    TopologyObject *result = NULL;
    blkid_topology topology = NULL;

    if (!probe) {
        PyErr_SetString(PyExc_RuntimeError, "internal error");
        return NULL;
    }

    topology = blkid_probe_get_topology (probe);
    if (!topology) {
        PyErr_SetString (PyExc_RuntimeError, "Failed to get topology");
        return NULL;
    }

    result = PyObject_New (TopologyObject, &TopologyType);
    if (!result) {
        PyErr_SetString (PyExc_MemoryError, "Failed to create a new Topology object");
        return NULL;
    }
    Py_INCREF (result);

    result->topology = topology;

    return (PyObject *) result;
}

static PyObject *Topology_get_alignment_offset (TopologyObject *self, PyObject *Py_UNUSED (ignored)) {
    unsigned long alignment_offset = blkid_topology_get_alignment_offset (self->topology);

    return PyLong_FromUnsignedLong (alignment_offset);
}

static PyObject *Topology_get_logical_sector_size (TopologyObject *self, PyObject *Py_UNUSED (ignored)) {
    unsigned long logical_sector_size = blkid_topology_get_logical_sector_size (self->topology);

    return PyLong_FromUnsignedLong (logical_sector_size);
}

static PyObject *Topology_get_minimum_io_size (TopologyObject *self, PyObject *Py_UNUSED (ignored)) {
    unsigned long minimum_io_size = blkid_topology_get_minimum_io_size (self->topology);

    return PyLong_FromUnsignedLong (minimum_io_size);
}

static PyObject *Topology_get_optimal_io_size (TopologyObject *self, PyObject *Py_UNUSED (ignored)) {
    unsigned long optimal_io_size = blkid_topology_get_optimal_io_size (self->topology);

    return PyLong_FromUnsignedLong (optimal_io_size);
}

static PyObject *Topology_get_physical_sector_size (TopologyObject *self, PyObject *Py_UNUSED (ignored)) {
    unsigned long physical_sector_size = blkid_topology_get_physical_sector_size (self->topology);

    return PyLong_FromUnsignedLong (physical_sector_size);
}

#ifdef HAVE_BLKID2360
static PyObject *Topology_get_dax (TopologyObject *self, PyObject *Py_UNUSED (ignored)) {
    int dax = blkid_topology_get_dax (self->topology);

    if (dax == 1)
        Py_RETURN_TRUE;
    else
        Py_RETURN_FALSE;
}
#endif

static PyGetSetDef Topology_getseters[] = {
    {"alignment_offset", (getter) Topology_get_alignment_offset, NULL, "alignment offset in bytes or 0", NULL},
    {"logical_sector_size", (getter) Topology_get_logical_sector_size, NULL, "logical sector size (BLKSSZGET ioctl) in bytes or 0", NULL},
    {"minimum_io_size", (getter) Topology_get_minimum_io_size, NULL, "minimum io size in bytes or 0", NULL},
    {"optimal_io_size", (getter) Topology_get_optimal_io_size, NULL, "optimal io size in bytes or 0", NULL},
    {"physical_sector_size", (getter) Topology_get_physical_sector_size, NULL, "logical sector size (BLKSSZGET ioctl) in bytes or 0", NULL},
#ifdef HAVE_BLKID2360
    {"dax", (getter) Topology_get_dax, NULL, "whether DAX is supported or not", NULL},
#endif
    {NULL, NULL, NULL, NULL, NULL}
};

PyTypeObject TopologyType = {
    PyVarObject_HEAD_INIT (NULL, 0)
    .tp_name = "blkid.Topology",
    .tp_basicsize = sizeof (TopologyObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = Topology_new,
    .tp_dealloc = (destructor) Topology_dealloc,
    .tp_init = (initproc) Topology_init,
    .tp_getset = Topology_getseters,
};
