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
 * Portions created by the Initial Developer are Copyright (C) 1998-2001
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

#ifndef nsImapUndoTxn_h__
#define nsImapUndoTxn_h__

#include "nsIMsgFolder.h"
#include "nsImapCore.h"
#include "nsIImapService.h"
#include "nsIImapIncomingServer.h"
#include "nsIUrlListener.h"
#include "nsMsgTxn.h"
#include "MailNewsTypes.h"
#include "nsTArray.h"
#include "nsIMsgOfflineImapOperation.h"
#include "nsCOMPtr.h"
#include "nsWeakReference.h"

class nsImapMoveCopyMsgTxn : public nsMsgTxn, nsIUrlListener
{
public:

  nsImapMoveCopyMsgTxn();
  nsImapMoveCopyMsgTxn(nsIMsgFolder* srcFolder, nsTArray<nsMsgKey>* srcKeyArray,
                       const char* srcMsgIdString, nsIMsgFolder* dstFolder,
                       bool isMove);
  virtual ~nsImapMoveCopyMsgTxn();

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIURLLISTENER

  NS_IMETHOD UndoTransaction(void);
  NS_IMETHOD RedoTransaction(void);

  // helper
  nsresult SetCopyResponseUid(const char *msgIdString);
  nsresult GetSrcKeyArray(nsTArray<nsMsgKey>& srcKeyArray);
  void GetSrcMsgIds(nsCString &srcMsgIds) {srcMsgIds = m_srcMsgIdString;}
  nsresult AddDstKey(nsMsgKey aKey);
  nsresult UndoMailboxDelete();
  nsresult RedoMailboxDelete();
  nsresult Init(nsIMsgFolder* srcFolder, nsTArray<nsMsgKey>* srcKeyArray,
                const char* srcMsgIdString, nsIMsgFolder* dstFolder,
                bool idsAreUids, bool isMove);

protected:

  nsWeakPtr m_srcFolder;
  nsCOMPtr<nsISupportsArray> m_srcHdrs;
  nsTArray<nsMsgKey> m_dupKeyArray;
  nsTArray<nsMsgKey> m_srcKeyArray;
  nsTArray<nsCString> m_srcMessageIds;
  nsCString m_srcMsgIdString;
  nsWeakPtr m_dstFolder;
  nsCString m_dstMsgIdString;
  bool m_idsAreUids;
  bool m_isMove;
  bool m_srcIsPop3;
  nsTArray<PRUint32> m_srcSizeArray;
  // this is used when we chain urls for imap undo, since "this" needs
  // to be the listener, but the folder may need to also be notified.
  nsWeakPtr m_onStopListener;

  nsresult GetImapDeleteModel(nsIMsgFolder* aFolder, nsMsgImapDeleteModel *aDeleteModel);
};

class nsImapOfflineTxn : public nsImapMoveCopyMsgTxn
{
public:
  nsImapOfflineTxn(nsIMsgFolder* srcFolder, nsTArray<nsMsgKey>* srcKeyArray,
                   const char* srcMsgIdString,
                   nsIMsgFolder* dstFolder,
                   bool isMove,
                   nsOfflineImapOperationType opType,
                   nsIMsgDBHdr *srcHdr);
  virtual ~nsImapOfflineTxn();

  NS_IMETHOD UndoTransaction(void);
  NS_IMETHOD RedoTransaction(void);
  void SetAddFlags(bool addFlags) {m_addFlags = addFlags;}
  void SetFlags(PRUint32 flags) {m_flags = flags;}
protected:
  nsOfflineImapOperationType m_opType;
  nsCOMPtr <nsIMsgDBHdr> m_header;
  // these two are used to undo flag changes, which we don't currently do.
  bool m_addFlags;
  PRUint32 m_flags;
};


#endif
