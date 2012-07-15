/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#ifndef nsMailboxUrl_h__
#define nsMailboxUrl_h__

#include "nsIMailboxUrl.h"
#include "nsMsgMailNewsUrl.h"
#include "nsIFile.h"
#include "nsCOMPtr.h"
#include "MailNewsTypes.h"
#include "nsTArray.h"
#include "nsISupportsObsolete.h"

class nsMailboxUrl : public nsIMailboxUrl, public nsMsgMailNewsUrl, public nsIMsgMessageUrl, public nsIMsgI18NUrl
{
public:
  // nsIURI over-ride...
  NS_IMETHOD SetSpec(const nsACString &aSpec);
  NS_IMETHOD SetQuery(const nsACString &aQuery);

  // from nsIMailboxUrl:
  NS_IMETHOD SetMailboxParser(nsIStreamListener * aConsumer);
  NS_IMETHOD GetMailboxParser(nsIStreamListener ** aConsumer);
  NS_IMETHOD SetMailboxCopyHandler(nsIStreamListener *  aConsumer);
  NS_IMETHOD GetMailboxCopyHandler(nsIStreamListener ** aConsumer);

  NS_IMETHOD GetMessageKey(nsMsgKey* aMessageKey);
  NS_IMETHOD GetMessageSize(PRUint32 *aMessageSize);
  NS_IMETHOD SetMessageSize(PRUint32 aMessageSize);
  NS_IMPL_CLASS_GETSET(MailboxAction, nsMailboxAction, m_mailboxAction)
  NS_IMETHOD IsUrlType(PRUint32 type, bool *isType);
  NS_IMETHOD SetMoveCopyMsgKeys(nsMsgKey *keysToFlag, PRInt32 numKeys);
  NS_IMETHOD GetMoveCopyMsgHdrForIndex(PRUint32 msgIndex, nsIMsgDBHdr **msgHdr);
  NS_IMETHOD GetNumMoveCopyMsgs(PRUint32 *numMsgs);
  NS_IMPL_CLASS_GETSET(CurMoveCopyMsgIndex, PRUint32, m_curMsgIndex)

  NS_IMETHOD GetFolder(nsIMsgFolder **msgFolder);

  // nsIMsgMailNewsUrl override
  NS_IMETHOD Clone(nsIURI **_retval);

  // nsMailboxUrl
  nsMailboxUrl();
  virtual ~nsMailboxUrl();
  NS_DECL_NSIMSGMESSAGEURL
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIMSGI18NURL

protected:
  // protocol specific code to parse a url...
  virtual nsresult ParseUrl();
  nsresult GetMsgHdrForKey(nsMsgKey  msgKey, nsIMsgDBHdr ** aMsgHdr);

  // mailboxurl specific state
  nsCOMPtr<nsIStreamListener> m_mailboxParser;
  nsCOMPtr<nsIStreamListener> m_mailboxCopyHandler;

  nsMailboxAction m_mailboxAction; // the action this url represents...parse mailbox, display messages, etc.
  nsCOMPtr <nsILocalFile>  m_filePath;
  char *m_messageID;
  PRUint32 m_messageSize;
  nsMsgKey m_messageKey;
  nsCString m_file;
  // This is currently only set when we're doing something with a .eml file.
  // If that changes, we should change the name of this var.
  nsCOMPtr<nsIMsgDBHdr> m_dummyHdr;

  // used by save message to disk
  nsCOMPtr<nsIFile> m_messageFile;
  bool                  m_addDummyEnvelope;
  bool                  m_canonicalLineEnding;
  nsresult ParseSearchPart();

  // for multiple msg move/copy
  nsTArray<nsMsgKey> m_keys;
  PRInt32 m_curMsgIndex;

  // truncated message support
  nsCString m_originalSpec;
  nsCString mURI; // the RDF URI associated with this url.
  nsCString mCharsetOverride; // used by nsIMsgI18NUrl...
};

#endif // nsMailboxUrl_h__
