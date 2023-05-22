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

import sys
from setuptools import setup, Extension


pkgs = pkgconfig.list_all()
if "blkid" not in pkgs:
    print("Please install libblkid-dev or libblkid-devel")
    exit(1)

vers = sys.version_info
if f"python-{vers.major}.{vers.minor}" not in pkgs:
    print("Please install python3-dev or python3-devel")
    exit(1)


macros = []

if pkgconfig.installed("blkid", ">= 2.36"):
    macros.append(("HAVE_BLKID2360", "1"))


with open("README.md", "r") as f:
    long_description = f.read()


def main():
    setup(name="pylibblkid",
          version="0.2",
          description="Python interface for the libblkid C library",
          long_description=long_description,
          long_description_content_type="text/markdown",
          author="Vojtech Trefny",
          author_email="vtrefny@redhat.com",
          url="http://github.com/vojtechtrefny/pyblkid",
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
                                 extra_compile_args = ["-std=c99", "-Wall", "-Wextra", "-Werror"])],
          classifiers=["Development Status :: 4 - Beta",
                       "Intended Audience :: Developers",
                       "License :: OSI Approved :: GNU Lesser General Public License v2 or later (LGPLv2+)",
                       "Programming Language :: C",
                       "Programming Language :: Python :: 3",
                       "Operating System :: POSIX :: Linux"])

if __name__ == "__main__":
    main()
