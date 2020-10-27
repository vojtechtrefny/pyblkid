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

PYTHON ?= python3


default: all

all:
	@$(PYTHON) setup.py build

test: all
	@env PYTHONPATH=$$(find $$(pwd) -name "*.so" | head -n 1 | xargs dirname):src \
	$(PYTHON) -m unittest discover -v

run-ipython: all
	@env PYTHONPATH=$$(find $$(pwd) -name "*.so" | head -n 1 | xargs dirname):src i$(PYTHON)

run-root-ipython: all
	@sudo env PYTHONPATH=$$(find $$(pwd) -name "*.so" | head -n 1 | xargs dirname):src i$(PYTHON)

clean:
	-rm -r build
