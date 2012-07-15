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

#ifndef nsLocalUndoTxn_h__
#define nsLocalUndoTxn_h__

#include "msgCore.h"
#include "nsIMsgFolder.h"
#include "nsMailboxService.h"
#include "nsMsgTxn.h"
#include "MailNewsTypes.h"
#include "nsTArray.h"
#include "nsCOMPtr.h"
#include "nsIUrlListener.h"
#include "nsIWeakReference.h"
#include "nsIWeakReferenceUtils.h"

class nsLocalUndoFolderListener;

class nsLocalMoveCopyMsgTxn : public nsIFolderListener, public nsMsgTxn
{
public:
    nsLocalMoveCopyMsgTxn();
    virtual ~nsLocalMoveCopyMsgTxn();
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIFOLDERLISTENER

    // overloading nsITransaction methods
    NS_IMETHOD UndoTransaction(void);
    NS_IMETHOD RedoTransaction(void);

    // helper
    nsresult AddSrcKey(nsMsgKey aKey);
    nsresult AddSrcStatusOffset(PRUint32 statusOffset);
    nsresult AddDstKey(nsMsgKey aKey);
    nsresult AddDstMsgSize(PRUint32 msgSize);
    nsresult SetSrcFolder(nsIMsgFolder* srcFolder);
    nsresult GetSrcIsImap(bool *isImap);
    nsresult SetDstFolder(nsIMsgFolder* dstFolder);
    nsresult Init(nsIMsgFolder* srcFolder,
                  nsIMsgFolder* dstFolder, bool isMove);
    nsresult UndoImapDeleteFlag(nsIMsgFolder* aFolder,
                                nsTArray<nsMsgKey>& aKeyArray,
                                bool deleteFlag);
    nsresult UndoTransactionInternal();
    // If the store using this undo transaction can "undelete" a message,
    // it will call this function on the transaction; This makes undo/redo
    // easy because message keys don't change after undo/redo. Otherwise,
    // we need to adjust the src or dst keys after every undo/redo action
    // to note the new keys.
    void SetCanUndelete(bool canUndelete) {m_canUndelete = canUndelete;}

private:
    nsWeakPtr m_srcFolder;
    nsTArray<nsMsgKey> m_srcKeyArray; // used when src is local or imap
    nsTArray<PRUint32> m_srcStatusOffsetArray; // used when src is local
    nsWeakPtr m_dstFolder;
    nsTArray<nsMsgKey> m_dstKeyArray;
    bool m_isMove;
    bool m_srcIsImap4;
    bool m_canUndelete;
    nsTArray<PRUint32> m_dstSizeArray;
    bool m_undoing; // if false, re-doing
    PRInt32 m_numHdrsCopied;
    nsTArray<nsCString> m_copiedMsgIds;
    nsLocalUndoFolderListener *mUndoFolderListener;
};

class nsLocalUndoFolderListener : public nsIFolderListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIFOLDERLISTENER

  nsLocalUndoFolderListener(nsLocalMoveCopyMsgTxn *aTxn, nsIMsgFolder *aFolder);
  virtual ~nsLocalUndoFolderListener();

private:
  nsLocalMoveCopyMsgTxn *mTxn;
  nsIMsgFolder *mFolder;
};

#endif
