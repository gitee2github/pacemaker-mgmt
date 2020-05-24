#
# heartbeat: Linux-HA heartbeat code
#
# Copyright (C) 2001 Michael Moerz
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
#
MAINTAINERCLEANFILES    = Makefile.in


halibdir		= $(libdir)/@HB_PKG@
guilibdir              = @HA_DATADIR@/heartbeat-gui
guilib_SCRIPTS         = haclient.py mgmtcmd.py license_register.py nkha_constants.py \
                         alarm.py   drbd.py  importview.py  nethb.py  script.py  sfex.py    sync.py \
                         diskhb.py  gfs.py   mirror.py  qdisk.py  serial.py  basefunc.py  upgrade.py
guilib_DATA            = ha.png login.png logout.png exit.png\
			  standby-node.png active-node.png add-resource.png remove-resource.png \
			  start-resource.png stop-resource.png master-resource.png slave-resource.png \
			  default-resource.png up-resource.png down-resource.png cleanup-resource.png \
			  dirty-stopped.png failed.png running.png stopped.png starting.png stopping.png \
			  nodes.png node.png administration.png record-pending.png perf.png switch.png \
			  haclient.glade drbd_logo.png pingd.png alert.png sfex.png addnode.png addhb.png mainnode.png \
			  rscattr-icon.png rename.png globalparam.png move.png migrate.png unmigrate.png \
                          constraint.png stock_about_24.png stock_info_24.png stock_key_24.png update.png \
			  M.png R.png S.png N.png MS.png NS.png M1.gif R1.gif S1.gif MS1.gif \
			  diskhb.png report.png transition.png conste.png script.png script ssh.expect device.sh \
			  manage-resource.png unmanage-resource.png sync.sh import.png delete-node.png gfs_init.sh

desktopdir		= $(datadir)/applications
desktop_DATA		= crm_gui.desktop

INCLUDES 	      	= -I$(top_builddir)/include  -I$(top_srcdir)/include \
			  -I$(top_builddir)/linux-ha -I$(top_srcdir)/linux-ha \
			  -I$(top_builddir)/libltdl -I$(top_srcdir)/libltdl

MGMT_DIR		= mgmt

halib_PROGRAMS 		= mgmtdtest
mgmtdtest_SOURCES  	= mgmtdtest.c
mgmtdtest_LDADD 	= $(top_builddir)/lib/$(MGMT_DIR)/libhbmgmtclient.la \
			  $(top_builddir)/lib/$(MGMT_DIR)/libhbmgmttls.la \
			  $(top_builddir)/lib/$(MGMT_DIR)/libhbmgmtcommon.la
mgmtdtest_LDFLAGS 	= $(GNUTLSLIBS)

EXTRA_DIST		= $(guilib_DATA)

