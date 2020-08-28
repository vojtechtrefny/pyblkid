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

#define UNUSED __attribute__((unused))


PyObject *Probe_new (PyTypeObject *type,  PyObject *args UNUSED, PyObject *kwargs UNUSED) {
    ProbeObject *self = (ProbeObject*) type->tp_alloc (type, 0);

    if (self)
        self->probe = NULL;

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

    blkid_free_probe (self->probe);
    Py_TYPE (self)->tp_free ((PyObject *) self);
}

static PyMethodDef Probe_methods[] = {
    {NULL, NULL, 0, NULL},
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
};
