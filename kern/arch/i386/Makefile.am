# Copyright (C) 2016 David Gao <davidgao1001@gmail.com>
# Copyright (C) 2016 Xiaofei Bai <xffbai@gmail.com>
#
# This file is part of AIMv6.
#
# AIMv6 is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# AIMv6 is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

include $(top_srcdir)/env.am

noinst_LTLIBRARIES = \
	libentry.la \
	libi386.la
noinst_DATA = vmaim.lds

CLEANFILES = vmaim.lds

libentry_la_SOURCES = entry.S

libi386_la_SOURCES = arch_init.c mm.c util.c trap.c trapentry.S vectors.S \
		     pgtable.c sync.c

vectors.S: $(top_srcdir)/tools/arch/i386/vectors.pl
	perl -w $^ >$@

vmaim.lds: vmaim.lds.S
	$(COMPILE) -E -P $^ -o $@

