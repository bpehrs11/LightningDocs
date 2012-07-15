# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is the MailNews Test Suite.
#
# The Initial Developer of the Original Code is
# the Mozilla Foundation.
# Portions created by the Initial Developer are Copyright (C) 2009
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Mark Banner <bugzilla@standard8.plus.com> (Initial Code)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

APP_NAME := $(MOZ_APP_DISPLAYNAME)

ifdef MOZ_DEBUG
APP_NAME := $(APP_NAME)Debug
endif

SYMBOLS_PATH := --symbols-path=$(DIST)/crashreporter-symbols

# BLOAT_EXTRA_ARG lets an application add an extra startup argument.
ifdef BLOAT_EXTRA_ARG
BLOAT_EXTRA_STARTUP_ARG := --extra-startup-arg=$(BLOAT_EXTRA_ARG)
endif

mailbloat:
	$(PYTHON) -u $(topsrcdir)/mozilla/config/pythonpath.py \
	  -I$(DIST)/../build -I$(MOZILLA_DIR)/build \
	  $(topsrcdir)/mailnews/test/performance/bloat/runtest.py \
	    --distdir=$(DIST) --bin=$(MOZ_APP_NAME) --brand=$(APP_NAME) \
	    $(SYMBOLS_PATH) $(BLOAT_EXTRA_STARTUP_ARG)
