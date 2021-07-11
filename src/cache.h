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
#ifndef CACHE_H
#define CACHE_H

#include <Python.h>

#include <blkid/blkid.h>

typedef struct {
    PyObject_HEAD
    blkid_cache cache;
} CacheObject;

extern PyTypeObject CacheType;

PyObject *Cache_new (PyTypeObject *type,  PyObject *args, PyObject *kwargs);
int Cache_init (CacheObject *self, PyObject *args, PyObject *kwargs);
void Cache_dealloc (CacheObject *self);

typedef struct {
    PyObject_HEAD
    blkid_dev device;
} DeviceObject;

extern PyTypeObject DeviceType;

#endif /* CACHE_H */
