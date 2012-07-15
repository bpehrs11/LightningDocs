#! /bin/sh
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
# The Original Code is the the Mozilla build system
#
# The Initial Developer of the Original Code is
# Ben Turner <mozilla@songbirdnest.com>
#
# Portions created by the Initial Developer are Copyright (C) 2007
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
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

add_makefiles "
mailnews/Makefile
mailnews/addrbook/Makefile
mailnews/addrbook/public/Makefile
mailnews/addrbook/src/Makefile
mailnews/addrbook/test/Makefile
mailnews/base/Makefile
mailnews/base/ispdata/Makefile
mailnews/base/public/Makefile
mailnews/base/search/public/Makefile
mailnews/base/search/src/Makefile
mailnews/base/src/Makefile
mailnews/base/test/Makefile
mailnews/base/util/Makefile
mailnews/build/Makefile
mailnews/compose/Makefile
mailnews/compose/public/Makefile
mailnews/compose/src/Makefile
mailnews/compose/test/Makefile
mailnews/db/Makefile
mailnews/db/gloda/Makefile
mailnews/db/gloda/components/Makefile
mailnews/db/gloda/modules/Makefile
mailnews/db/gloda/test/Makefile
mailnews/db/msgdb/Makefile
mailnews/db/msgdb/public/Makefile
mailnews/db/msgdb/src/Makefile
mailnews/db/msgdb/test/Makefile
mailnews/extensions/Makefile
mailnews/extensions/bayesian-spam-filter/Makefile
mailnews/extensions/bayesian-spam-filter/src/Makefile
mailnews/extensions/bayesian-spam-filter/test/Makefile
mailnews/extensions/fts3/Makefile
mailnews/extensions/fts3/public/Makefile
mailnews/extensions/fts3/src/Makefile
mailnews/extensions/mailviews/Makefile
mailnews/extensions/mailviews/public/Makefile
mailnews/extensions/mailviews/content/Makefile
mailnews/extensions/mailviews/src/Makefile
mailnews/extensions/mdn/Makefile
mailnews/extensions/mdn/src/Makefile
mailnews/extensions/newsblog/Makefile
mailnews/extensions/offline-startup/Makefile
mailnews/extensions/smime/Makefile
mailnews/extensions/smime/public/Makefile
mailnews/extensions/smime/src/Makefile
mailnews/imap/Makefile
mailnews/imap/public/Makefile
mailnews/imap/src/Makefile
mailnews/imap/test/Makefile
mailnews/import/Makefile
mailnews/import/applemail/src/Makefile
mailnews/import/build/Makefile
mailnews/import/eudora/src/Makefile
mailnews/import/oexpress/Makefile
mailnews/import/outlook/src/Makefile
mailnews/import/public/Makefile
mailnews/import/src/Makefile
mailnews/import/test/Makefile
mailnews/import/text/src/Makefile
mailnews/import/vcard/src/Makefile
mailnews/import/winlivemail/Makefile
mailnews/local/Makefile
mailnews/local/public/Makefile
mailnews/local/src/Makefile
mailnews/local/test/Makefile
mailnews/mapi/mapiDll/Makefile
mailnews/mapi/mapihook/Makefile
mailnews/mapi/mapihook/build/Makefile
mailnews/mapi/mapihook/public/Makefile
mailnews/mapi/mapihook/src/Makefile
mailnews/mime/Makefile
mailnews/mime/cthandlers/Makefile
mailnews/mime/cthandlers/calendar/Makefile
mailnews/mime/cthandlers/glue/Makefile
mailnews/mime/cthandlers/smimestub/Makefile
mailnews/mime/cthandlers/vcard/Makefile
mailnews/mime/emitters/Makefile
mailnews/mime/emitters/src/Makefile
mailnews/mime/public/Makefile
mailnews/mime/src/Makefile
mailnews/mime/test/Makefile
mailnews/news/Makefile
mailnews/news/public/Makefile
mailnews/news/src/Makefile
mailnews/news/test/Makefile
mailnews/test/performance/bloat/Makefile
"
