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
# The Original Code is Mozilla Build System
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

MOZ_APP_BASENAME=SeaMonkey
MOZ_APP_VENDOR=Mozilla
MOZ_APP_NAME=seamonkey
MOZ_APP_DISPLAYNAME=SeaMonkey
if [ "$COMM_BUILD" ]; then
  MOZ_LDAP_XPCOM=1
fi
MOZ_CHROME_FILE_FORMAT=omni
MOZ_COMPOSER=1
MOZ_SUITE=1
MOZ_BRANDING_DIRECTORY=suite/branding/nightly
MOZ_OFFICIAL_BRANDING_DIRECTORY=suite/branding/nightly
MOZ_EXTENSIONS_DEFAULT=" venkman inspector irc gnomevfs"
MOZ_UPDATER=1
MOZ_HELP_VIEWER=1
MOZ_MORK=1
MOZ_STATIC_BUILD_UNSUPPORTED=1
MOZ_APP_COMPONENT_LIBS="xpautocomplete $MAIL_COMPONENT $LDAP_COMPONENT $MORK_COMPONENT"
MOZ_APP_COMPONENT_MODULES="MODULE(xpAutoComplete) $MAIL_MODULE $LDAP_MODULE $MORK_MODULE"
MOZ_APP_EXTRA_LIBS="$LDAP_LIBS"
MOZ_SERVICES_SYNC=1

MOZ_APP_VERSION_TXT=${_topsrcdir}/$MOZ_BUILD_APP/config/version.txt
MOZ_APP_VERSION=`cat $MOZ_APP_VERSION_TXT`
SEAMONKEY_VERSION=$MOZ_APP_VERSION

MOZ_APP_ID={92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}
MOZ_PROFILE_MIGRATOR=1
MOZ_EXTENSION_MANAGER=1
MOZ_APP_STATIC_INI=1
