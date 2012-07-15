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
 *    Prasad Sunkari <prasad@medhas.org>
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

#ifndef nsMsgTxn_h__
#define nsMsgTxn_h__

#include "nsITransaction.h"
#include "msgCore.h"
#include "nsCOMPtr.h"
#include "nsIMsgWindow.h"
#include "nsInterfaceHashtable.h"
#include "MailNewsTypes2.h"
#include "nsIVariant.h"
#include "nsIWritablePropertyBag.h"
#include "nsIWritablePropertyBag2.h"

#define NS_MESSAGETRANSACTION_IID \
{ /* da621b30-1efc-11d3-abe4-00805f8ac968 */ \
    0xda621b30, 0x1efc, 0x11d3, \
  { 0xab, 0xe4, 0x00, 0x80, 0x5f, 0x8a, 0xc9, 0x68 } }
/**
 * base class for all message undo/redo transactions.
 */

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_DEFAULT

class NS_MSG_BASE nsMsgTxn : public nsITransaction, 
                             public nsIWritablePropertyBag,
                             public nsIWritablePropertyBag2
{
public:
    nsMsgTxn();
    virtual ~nsMsgTxn();

    nsresult Init();

    NS_IMETHOD DoTransaction(void);

    NS_IMETHOD UndoTransaction(void) = 0;

    NS_IMETHOD RedoTransaction(void) = 0;
    
    NS_IMETHOD GetIsTransient(bool *aIsTransient);

    NS_IMETHOD Merge(nsITransaction *aTransaction, bool *aDidMerge);

    nsresult GetMsgWindow(nsIMsgWindow **msgWindow);
    nsresult SetMsgWindow(nsIMsgWindow *msgWindow);
    nsresult SetTransactionType(PRUint32 txnType);
 
    NS_DECL_ISUPPORTS
    NS_DECL_NSIPROPERTYBAG
    NS_DECL_NSIPROPERTYBAG2
    NS_DECL_NSIWRITABLEPROPERTYBAG
    NS_DECL_NSIWRITABLEPROPERTYBAG2

protected:
    // a hash table of string -> nsIVariant
    nsInterfaceHashtable<nsStringHashKey, nsIVariant> mPropertyHash;
    nsCOMPtr<nsIMsgWindow> m_msgWindow;
    PRUint32 m_txnType;
    nsresult CheckForToggleDelete(nsIMsgFolder *aFolder, const nsMsgKey &aMsgKey, bool *aResult);
};

#undef  IMETHOD_VISIBILITY
#define IMETHOD_VISIBILITY NS_VISIBILITY_HIDDEN

#endif
