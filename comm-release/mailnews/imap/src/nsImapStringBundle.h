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
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Lorenzo Colitti <lorenzo@colitti.com>
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

#ifndef _nsImapStringBundle_H__
#define _nsImapStringBundle_H__

#include "nsIStringBundle.h"

PR_BEGIN_EXTERN_C

nsresult      IMAPGetStringByID(PRInt32 stringID, PRUnichar **aString);
nsresult      IMAPGetStringBundle(nsIStringBundle **aBundle);

PR_END_EXTERN_C

#define	IMAP_STATUS_SELECTING_MAILBOX                                  5000
#define	IMAP_STATUS_CREATING_MAILBOX                                  5001
#define	IMAP_STATUS_DELETING_MAILBOX                                  5002
#define	IMAP_STATUS_RENAMING_MAILBOX                                  5003
#define	IMAP_STATUS_LOOKING_FOR_MAILBOX                                  5004
#define	IMAP_STATUS_SUBSCRIBE_TO_MAILBOX                                  5005
#define	IMAP_STATUS_UNSUBSCRIBE_MAILBOX                                  5006
#define	IMAP_STATUS_SEARCH_MAILBOX                                  5007
#define	IMAP_STATUS_MSG_INFO                                  5008
#define	IMAP_STATUS_CLOSE_MAILBOX                                  5009
#define	IMAP_STATUS_EXPUNGING_MAILBOX                                  5010
#define	IMAP_STATUS_LOGGING_OUT                                  5011
#define	IMAP_STATUS_CHECK_COMPAT                                  5012
#define	IMAP_STATUS_SENDING_LOGIN                                  5013
#define	IMAP_STATUS_SENDING_AUTH_LOGIN                                  5014
#define	IMAP_DOWNLOADING_MESSAGE                                  5015
#define	IMAP_GETTING_ACL_FOR_FOLDER                                  5029
#define	IMAP_GETTING_SERVER_INFO                                  5030
#define	IMAP_GETTING_MAILBOX_INFO                                  5031
#define	IMAP_EMPTY_MIME_PART                                  5032
#define	IMAP_RECEIVING_MESSAGE_HEADERS_OF                                  5036
#define	IMAP_RECEIVING_MESSAGE_FLAGS_OF                                  5037
#define	IMAP_DELETING_MESSAGES                                  5038
#define	IMAP_DELETING_MESSAGE                   5039
#define	IMAP_MOVING_MESSAGES_TO                 5040
#define	IMAP_MOVING_MESSAGE_TO                  5041
#define	IMAP_COPYING_MESSAGES_TO                5042
#define	IMAP_COPYING_MESSAGE_TO                 5043
#define IMAP_FOLDER_RECEIVING_MESSAGE_OF        5045
#define IMAP_DISCOVERING_MAILBOX                5046
#define IMAP_ENTER_PASSWORD_PROMPT              5047
#define IMAP_SERVER_NOT_IMAP4                   5048
#define IMAP_SERVER_SAID                        5049
#define IMAP_DONE                               5050
#define IMAP_ENTER_PASSWORD_PROMPT_TITLE        5051
#define IMAP_UNKNOWN_HOST_ERROR                 5052
#define IMAP_CONNECTION_REFUSED_ERROR               5053
#define IMAP_NET_TIMEOUT_ERROR                      5054
#define IMAP_NO_NEW_MESSAGES                        5056
#define IMAP_DEFAULT_ACCOUNT_NAME                   5057
#define IMAP_EMPTY_TRASH_CONFIRM                    5061
#define IMAP_SPECIAL_CHAR                           5065
#define IMAP_PERSONAL_SHARED_FOLDER_TYPE_NAME       5066
#define IMAP_PUBLIC_FOLDER_TYPE_NAME                5067
#define IMAP_OTHER_USERS_FOLDER_TYPE_NAME           5068
#define IMAP_PERSONAL_FOLDER_TYPE_DESCRIPTION       5069
#define IMAP_PERSONAL_SHARED_FOLDER_TYPE_DESCRIPTION 5070
#define IMAP_PUBLIC_FOLDER_TYPE_DESCRIPTION         5071
#define IMAP_OTHER_USERS_FOLDER_TYPE_DESCRIPTION    5072
#define IMAP_ACL_FULL_RIGHTS                        5073
#define IMAP_ACL_LOOKUP_RIGHT                       5074
#define IMAP_ACL_READ_RIGHT                         5075
#define IMAP_ACL_SEEN_RIGHT                         5076
#define IMAP_ACL_WRITE_RIGHT                        5077
#define IMAP_ACL_INSERT_RIGHT                       5078
#define IMAP_ACL_POST_RIGHT                         5079
#define IMAP_ACL_CREATE_RIGHT                       5080
#define IMAP_ACL_DELETE_RIGHT                       5081
#define IMAP_ACL_ADMINISTER_RIGHT                   5082
#define IMAP_SERVER_DOESNT_SUPPORT_ACL              5084
#define IMAP_ACL_EXPUNGE_RIGHT                      5085
#define IMAP_SERVER_DISCONNECTED                    5090
#define IMAP_SUBSCRIBE_PROMPT                       5092
#define IMAP_SERVER_DROPPED_CONNECTION              5093
#define IMAP_QUOTA_STATUS_FOLDERNOTOPEN             5095
#define IMAP_QUOTA_STATUS_NOTSUPPORTED              5096
#define IMAP_QUOTA_STATUS_NOQUOTA                   5097
#define IMAP_OUT_OF_MEMORY                          5100
#define IMAP_COPYING_MESSAGE_OF                     5103
#define IMAP_MOVE_FOLDER_TO_TRASH                   5105
#define IMAP_DELETE_NO_TRASH                        5106
#define IMAP_DELETE_FOLDER_DIALOG_TITLE             5107
#define IMAP_DELETE_FOLDER_BUTTON_LABEL             5108
#define IMAP_AUTH_CHANGE_ENCRYPT_TO_PLAIN_SSL       5110
#define IMAP_AUTH_CHANGE_PLAIN_TO_ENCRYPT           5111
#define IMAP_AUTH_CHANGE_ENCRYPT_TO_PLAIN_NO_SSL    5112
#define IMAP_AUTH_MECH_NOT_SUPPORTED                5113
#define IMAP_AUTH_MECH_FAILED                       5114
#define IMAP_AUTH_GSSAPI_FAILED                     5115
#define IMAP_LOGIN_DISABLED                         5116
#define IMAP_SERVER_COMMAND_FAILED                  5117
#define IMAP_FOLDER_COMMAND_FAILED                  5118
#define IMAP_SERVER_ALERT                           5119
#endif /* _nsImapStringBundle_H__ */
