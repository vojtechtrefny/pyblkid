# Copyright (C) 2020  Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

import pkgconfig

from setuptools import setup, Extension


macros = []

if pkgconfig.installed("blkid", ">= 2.36"):
    macros.append(("HAVE_BLKID2360", "1"))


def main():
    setup(name="pyblkid",
          version="0.1",
          description="Python interface for the libblkid C library",
          author="Vojtech Trefny",
          author_email="vtrefny@redhat.com",
          ext_modules=[Extension("blkid",
                                 sources = ["src/pyblkid.c",
                                            "src/topology.c",
                                            "src/partitions.c",
                                            "src/cache.c",
                                            "src/probe.c",],
                                 include_dirs = ["/usr/include"],
                                 libraries = ["blkid"],
                                 library_dirs = ["/usr/lib"],
                                 define_macros=macros,
                                 extra_compile_args = ["-Wall", "-Wextra", "-Werror"])])

if __name__ == "__main__":
    main()
