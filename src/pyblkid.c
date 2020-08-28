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

#include <blkid/blkid.h>

static PyMethodDef BlkidMethods[] = {
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

    module = PyModule_Create (&blkidmodule);
    if (!module)
        return NULL;

    return module;
}
