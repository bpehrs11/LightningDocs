/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 *   Olivier Parniere BT Global Services / Etat francais Ministere de la Defense
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

#ifndef _MsgCompFields_H_
#define _MsgCompFields_H_

#include "nsIMsgCompFields.h"
#include "msgCore.h"
#include "nsIAbCard.h"
#include "nsTArray.h"
#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"

struct nsMsgRecipient
{
  nsMsgRecipient() : mPreferFormat(nsIAbPreferMailFormat::unknown),
                     mProcessed(false) {}

  nsMsgRecipient(const nsMsgRecipient &other)
  {
    mAddress = other.mAddress;
    mEmail = other.mEmail;
    mPreferFormat = other.mPreferFormat;
    mProcessed = other.mProcessed;
  }

  ~nsMsgRecipient() {}

  nsString mAddress;
  nsString mEmail;
  PRUint32 mPreferFormat;
  PRUint32 mProcessed;
};

/* Note that all the "Get" methods never return NULL (except in case of serious
   error, like an illegal parameter); rather, they return "" if things were set
   to NULL.  This makes it real handy for the callers. */

class nsMsgCompFields : public nsIMsgCompFields {
public:
  nsMsgCompFields();
  virtual ~nsMsgCompFields();

  /* this macro defines QueryInterface, AddRef and Release for this class */
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGCOMPFIELDS

  typedef enum MsgHeaderID
  {
    MSG_FROM_HEADER_ID        = 0,
    MSG_REPLY_TO_HEADER_ID,
    MSG_TO_HEADER_ID,
    MSG_CC_HEADER_ID,
    MSG_BCC_HEADER_ID,
    MSG_FCC_HEADER_ID,
    MSG_FCC2_HEADER_ID,
    MSG_NEWSGROUPS_HEADER_ID,
    MSG_FOLLOWUP_TO_HEADER_ID,
    MSG_SUBJECT_HEADER_ID,
    MSG_ATTACHMENTS_HEADER_ID,
    MSG_ORGANIZATION_HEADER_ID,
    MSG_REFERENCES_HEADER_ID,
    MSG_OTHERRANDOMHEADERS_HEADER_ID,
    MSG_NEWSPOSTURL_HEADER_ID,
    MSG_PRIORITY_HEADER_ID,
    MSG_CHARACTER_SET_HEADER_ID,
    MSG_MESSAGE_ID_HEADER_ID,
    MSG_X_TEMPLATE_HEADER_ID,
    MSG_DRAFT_ID_HEADER_ID,
    MSG_TEMPORARY_FILES_HEADER_ID,
    MSG_SENDERREPLY_HEADER_ID,
    MSG_ALLREPLY_HEADER_ID,
    MSG_LISTREPLY_HEADER_ID,
    
    MSG_MAX_HEADERS   //Must be the last one.
  } MsgHeaderID;

  nsresult SetAsciiHeader(MsgHeaderID header, const char *value);
  const char* GetAsciiHeader(MsgHeaderID header); //just return the address of the internal header variable, don't dispose it

  nsresult SetUnicodeHeader(MsgHeaderID header, const nsAString &value);
  nsresult GetUnicodeHeader(MsgHeaderID header, nsAString &_retval); 

  /* Convenience routines to get and set header's value...
  
    IMPORTANT:
    all routines const char* GetXxx(void) will return a pointer to the header, please don't free it.
  */

  nsresult SetFrom(const char *value) {return SetAsciiHeader(MSG_FROM_HEADER_ID, value);}
  const char* GetFrom(void) {return GetAsciiHeader(MSG_FROM_HEADER_ID);}

  nsresult SetReplyTo(const char *value) {return SetAsciiHeader(MSG_REPLY_TO_HEADER_ID, value);}
  const char* GetReplyTo() {return GetAsciiHeader(MSG_REPLY_TO_HEADER_ID);}

  nsresult SetTo(const char *value) {return SetAsciiHeader(MSG_TO_HEADER_ID, value);}
  const char* GetTo() {return GetAsciiHeader(MSG_TO_HEADER_ID);}

  nsresult SetCc(const char *value) {return SetAsciiHeader(MSG_CC_HEADER_ID, value);}
  const char* GetCc() {return GetAsciiHeader(MSG_CC_HEADER_ID);}

  nsresult SetBcc(const char *value) {return SetAsciiHeader(MSG_BCC_HEADER_ID, value);}
  const char* GetBcc() {return GetAsciiHeader(MSG_BCC_HEADER_ID);}

  nsresult SetFcc(const char *value) {return SetAsciiHeader(MSG_FCC_HEADER_ID, value);}
  const char* GetFcc() {return GetAsciiHeader(MSG_FCC_HEADER_ID);}

  nsresult SetFcc2(const char *value) {return SetAsciiHeader(MSG_FCC2_HEADER_ID, value);}
  const char* GetFcc2() {return GetAsciiHeader(MSG_FCC2_HEADER_ID);}

  nsresult SetNewsgroups(const char *aValue) {return SetAsciiHeader(MSG_NEWSGROUPS_HEADER_ID, aValue);}
  const char* GetNewsgroups() {return GetAsciiHeader(MSG_NEWSGROUPS_HEADER_ID);}

  const char* GetNewshost() {return GetAsciiHeader(MSG_NEWSPOSTURL_HEADER_ID);}

  nsresult SetFollowupTo(const char *aValue) {return SetAsciiHeader(MSG_FOLLOWUP_TO_HEADER_ID, aValue);}
  const char* GetFollowupTo() {return GetAsciiHeader(MSG_FOLLOWUP_TO_HEADER_ID);}

  nsresult SetSubject(const char *value) {return SetAsciiHeader(MSG_SUBJECT_HEADER_ID, value);}
  const char* GetSubject() {return GetAsciiHeader(MSG_SUBJECT_HEADER_ID);}

  const char* GetAttachments() {
    NS_ERROR("nsMsgCompFields::GetAttachments is not supported anymore, please use nsMsgCompFields::GetAttachmentsArray");
    return GetAsciiHeader(MSG_ATTACHMENTS_HEADER_ID);
    }

  const char* GetTemporaryFiles() {
    NS_ERROR("nsMsgCompFields::GetTemporaryFiles is not supported anymore, please use nsMsgCompFields::GetAttachmentsArray");
    return GetAsciiHeader(MSG_TEMPORARY_FILES_HEADER_ID);
    }

  nsresult SetOrganization(const char *value) {return SetAsciiHeader(MSG_ORGANIZATION_HEADER_ID, value);}
  const char* GetOrganization() {return GetAsciiHeader(MSG_ORGANIZATION_HEADER_ID);}

  const char* GetReferences() {return GetAsciiHeader(MSG_REFERENCES_HEADER_ID);}

  nsresult SetOtherRandomHeaders(const char *value) {return SetAsciiHeader(MSG_OTHERRANDOMHEADERS_HEADER_ID, value);}
  const char* GetOtherRandomHeaders() {return GetAsciiHeader(MSG_OTHERRANDOMHEADERS_HEADER_ID);}

  nsresult SetSenderReply(const char *value) {return SetAsciiHeader(MSG_SENDERREPLY_HEADER_ID, value);}
  const char* GetSenderReply() {return GetAsciiHeader(MSG_SENDERREPLY_HEADER_ID);}

  nsresult SetAllReply(const char *value) {return SetAsciiHeader(MSG_ALLREPLY_HEADER_ID, value);}
  const char* GetAllReply() {return GetAsciiHeader(MSG_ALLREPLY_HEADER_ID);}

  nsresult SetListReply(const char *value) {return SetAsciiHeader(MSG_LISTREPLY_HEADER_ID, value);}
  const char* GetListReply() {return GetAsciiHeader(MSG_LISTREPLY_HEADER_ID);}

  const char* GetNewspostUrl() {return GetAsciiHeader(MSG_NEWSPOSTURL_HEADER_ID);}

  const char* GetPriority() {return GetAsciiHeader(MSG_PRIORITY_HEADER_ID);}

  const char* GetCharacterSet() {return GetAsciiHeader(MSG_CHARACTER_SET_HEADER_ID);}

  const char* GetMessageId() {return GetAsciiHeader(MSG_MESSAGE_ID_HEADER_ID);}

  nsresult SetTemplateName(const char *value) {return SetAsciiHeader(MSG_X_TEMPLATE_HEADER_ID, value);}
  const char* GetTemplateName() {return GetAsciiHeader(MSG_X_TEMPLATE_HEADER_ID);}

  const char* GetDraftId() {return GetAsciiHeader(MSG_DRAFT_ID_HEADER_ID);}

  bool GetReturnReceipt() {return m_returnReceipt;}
  bool GetDSN() {return m_DSN;}
  bool GetAttachVCard() {return m_attachVCard;}
  bool GetForcePlainText() {return m_forcePlainText;}
  bool GetUseMultipartAlternative() {return m_useMultipartAlternative;}
  bool GetBodyIsAsciiOnly() {return m_bodyIsAsciiOnly;}
  bool GetForceMsgEncoding() {return m_forceMsgEncoding;}

  nsresult SetBody(const char *value);
  const char* GetBody();

  nsresult SplitRecipientsEx(const nsAString &recipients,
                             nsTArray<nsMsgRecipient> &aResult); 

protected:
  char*       m_headers[MSG_MAX_HEADERS];
  nsCString   m_body;
  nsCOMArray<nsIMsgAttachment> m_attachments;
  bool        m_attachVCard;
  bool        m_forcePlainText;
  bool        m_useMultipartAlternative;
  bool        m_returnReceipt;
  bool        m_DSN;
  bool        m_bodyIsAsciiOnly;
  bool        m_forceMsgEncoding;
  PRInt32     m_receiptHeaderType;        /* receipt header type */
  nsCString   m_DefaultCharacterSet;
  bool        m_needToCheckCharset;

  nsCOMPtr<nsISupports> mSecureCompFields;
};


#endif /* _MsgCompFields_H_ */
