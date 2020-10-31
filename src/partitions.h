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
#ifndef PARTITIONS_H
#define PARTITIONS_H

#include <Python.h>

#include <blkid/blkid.h>

typedef struct {
    PyObject_HEAD
    blkid_partlist partlist;
    PyObject *Parttable_object;
} PartlistObject;

extern PyTypeObject PartlistType;

PyObject *Partlist_new (PyTypeObject *type,  PyObject *args, PyObject *kwargs);
int Partlist_init (PartlistObject *self, PyObject *args, PyObject *kwargs);
void Partlist_dealloc (PartlistObject *self);

PyObject *_Partlist_get_partlist_object (blkid_probe probe);


typedef struct {
    PyObject_HEAD
    blkid_parttable table;
} ParttableObject;

extern PyTypeObject ParttableType;

PyObject *Parttable_new (PyTypeObject *type,  PyObject *args, PyObject *kwargs);
int Parttable_init (ParttableObject *self, PyObject *args, PyObject *kwargs);
void Parttable_dealloc (ParttableObject *self);

PyObject *_Parttable_get_parttable_object (blkid_partlist partlist);

typedef struct {
    PyObject_HEAD
    int number;
    blkid_partition partition;
} PartitionObject;

extern PyTypeObject PartitionType;

PyObject *Partition_new (PyTypeObject *type,  PyObject *args, PyObject *kwargs);
int Partition_init (PartitionObject *self, PyObject *args, PyObject *kwargs);
void Partition_dealloc (PartitionObject *self);

#endif /* PARTITIONS_H */
