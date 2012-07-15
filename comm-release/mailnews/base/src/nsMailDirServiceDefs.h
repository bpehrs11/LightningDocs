/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Mail Directory Provider.
 *
 * The Initial Developer of the Original Code is
 *   Scott MacGregor <mscott@mozilla.org>.
 * Portions created by the Initial Developer are Copyright (C) 2006
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */


#ifndef nsMailDirectoryServiceDefs_h___
#define nsMailDirectoryServiceDefs_h___

//=============================================================================
//
// Defines property names for directories available from the mail-specific
// nsMailDirProvider.
//
// System and XPCOM properties are defined in nsDirectoryServiceDefs.h.
// General application properties are defined in nsAppDirectoryServiceDefs.h.
//
//=============================================================================

// ----------------------------------------------------------------------------
// Files and directories that exist on a per-profile basis.
// ----------------------------------------------------------------------------

#define NS_APP_MAIL_50_DIR                      "MailD"
#define NS_APP_IMAP_MAIL_50_DIR                 "IMapMD"
#define NS_APP_NEWS_50_DIR                      "NewsD"

#define NS_APP_MESSENGER_FOLDER_CACHE_50_FILE   "MFCaF"

#define ISP_DIRECTORY_LIST                 "ISPDL"
      
#endif
