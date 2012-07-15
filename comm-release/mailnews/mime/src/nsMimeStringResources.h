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
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Netscape.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either of the GNU General Public License Version 2 or later (the "GPL"),
 * or the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifndef _NAME_OF_THIS_HEADER_FILE__
#define _NAME_OF_THIS_HEADER_FILE__

/* Note that the negative values are not actually strings: they are error
 * codes masquerading as strings. Do not pass them to MimeGetStringByID()
 * expecting to get anything back for your trouble.
 */
#define  MIME_OUT_OF_MEMORY                        -1000
#define  MIME_UNABLE_TO_OPEN_TMP_FILE              -1001
#define  MIME_ERROR_WRITING_FILE                   -1002
#define  MIME_MHTML_SUBJECT                        1000
#define  MIME_MHTML_RESENT_COMMENTS                1001
#define  MIME_MHTML_RESENT_DATE                    1002
#define  MIME_MHTML_RESENT_SENDER                  1003
#define  MIME_MHTML_RESENT_FROM                    1004
#define  MIME_MHTML_RESENT_TO                      1005
#define  MIME_MHTML_RESENT_CC                      1006
#define  MIME_MHTML_DATE                           1007
#define  MIME_MHTML_SENDER                         1008
#define  MIME_MHTML_FROM                           1009
#define  MIME_MHTML_REPLY_TO                       1010
#define  MIME_MHTML_ORGANIZATION                   1011
#define  MIME_MHTML_TO                             1012
#define  MIME_MHTML_CC                             1013
#define  MIME_MHTML_NEWSGROUPS                     1014
#define  MIME_MHTML_FOLLOWUP_TO                    1015
#define  MIME_MHTML_REFERENCES                     1016
#define  MIME_MHTML_MESSAGE_ID                     1021
#define  MIME_MHTML_BCC                            1023
#define  MIME_MSG_LINK_TO_DOCUMENT                 1026
#define  MIME_MSG_DOCUMENT_INFO                    1027
#define  MIME_MSG_ATTACHMENT                       1028
#define  MIME_MSG_DEFAULT_ATTACHMENT_NAME          1040
#define  MIME_FORWARDED_MESSAGE_HTML_USER_WROTE    1041

#endif /* _NAME_OF_THIS_HEADER_FILE__ */
