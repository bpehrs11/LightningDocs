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

#ifndef _nsMsgSendLater_H_
#define _nsMsgSendLater_H_

#include "nsCOMArray.h"
#include "nsIMsgFolder.h"
#include "nsIMsgSendListener.h"
#include "nsIMsgSendLaterListener.h"
#include "nsIMsgSendLater.h"
#include "nsIMsgStatusFeedback.h"
#include "nsTObserverArray.h"
#include "nsIObserver.h"
#include "nsITimer.h"
#include "nsIMsgShutdown.h"

////////////////////////////////////////////////////////////////////////////////////
// This is the listener class for the send operation. We have to create this class 
// to listen for message send completion and eventually notify the caller
////////////////////////////////////////////////////////////////////////////////////
class nsMsgSendLater;

class SendOperationListener : public nsIMsgSendListener,
                              public nsIMsgCopyServiceListener
{
public:
  SendOperationListener(nsMsgSendLater *aSendLater);
  virtual ~SendOperationListener();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGSENDLISTENER
  NS_DECL_NSIMSGCOPYSERVICELISTENER

private:
  nsMsgSendLater *mSendLater;
};

class nsMsgSendLater: public nsIMsgSendLater,
                      public nsIFolderListener,
                      public nsIObserver,
                      public nsIUrlListener,
                      public nsIMsgShutdownTask

{
public:
  nsMsgSendLater();
  virtual     ~nsMsgSendLater();
  nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMSGSENDLATER
  NS_DECL_NSIFOLDERLISTENER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIURLLISTENER
  NS_DECL_NSIMSGSHUTDOWNTASK

  // Methods needed for implementing interface...
  nsresult                  StartNextMailFileSend(nsresult prevStatus);
  nsresult                  CompleteMailFileSend();

  nsresult                  DeleteCurrentMessage();
  nsresult                  SetOrigMsgDisposition();
  // Necessary for creating a valid list of recipients
  nsresult                  BuildHeaders();
  nsresult                  DeliverQueuedLine(char *line, PRInt32 length);
  nsresult                  RebufferLeftovers(char *startBuf,  PRUint32 aLen);
  nsresult                  BuildNewBuffer(const char* aBuf, PRUint32 aCount, PRUint32 *totalBufSize);

  // methods for listener array processing...
  void NotifyListenersOnStartSending(PRUint32 aTotalMessageCount);
  void NotifyListenersOnMessageStartSending(PRUint32 aCurrentMessage,
                                            PRUint32 aTotalMessage,
                                            nsIMsgIdentity *aIdentity);
  void NotifyListenersOnProgress(PRUint32 aCurrentMessage,
                                 PRUint32 aTotalMessage,
                                 PRUint32 aSendPercent,
                                 PRUint32 aCopyPercent);
  void NotifyListenersOnMessageSendError(PRUint32 aCurrentMessage,
                                         nsresult aStatus,
                                         const PRUnichar *aMsg);
  void EndSendMessages(nsresult aStatus, const PRUnichar *aMsg, 
                       PRUint32 aTotalTried, PRUint32 aSuccessful);

  bool OnSendStepFinished(nsresult aStatus);
  void OnCopyStepFinished(nsresult aStatus);

  // counters and things for enumeration 
  PRUint32                  mTotalSentSuccessfully;
  PRUint32                  mTotalSendCount;
  nsCOMArray<nsIMsgDBHdr> mMessagesToSend;
  nsCOMPtr<nsISimpleEnumerator> mEnumerator;
  nsCOMPtr<nsIMsgFolder>    mMessageFolder;
  nsCOMPtr<nsIMsgStatusFeedback> mFeedback;
 
  // Private Information
private:
  nsresult GetIdentityFromKey(const char *aKey, nsIMsgIdentity **aIdentity);
  nsresult ReparseDBIfNeeded(nsIUrlListener *aListener);
  nsresult InternalSendMessages(bool aUserInitiated,
                                nsIMsgIdentity *aIdentity);

  nsTObserverArray<nsCOMPtr<nsIMsgSendLaterListener> > mListenerArray;
  nsCOMPtr<nsIMsgDBHdr> mMessage;
  nsCOMPtr<nsITimer> mTimer;
  bool mTimerSet;
  nsCOMPtr<nsIUrlListener> mShutdownListener;

  //
  // File output stuff...
  //
  nsCOMPtr<nsIFile>         mTempFile;
  nsCOMPtr<nsIOutputStream> mOutFile;

  void                      *mTagData;

  // For building headers and stream parsing...
  char                      *m_to;
  char                      *m_bcc;
  char                      *m_fcc;
  char                      *m_newsgroups;
  char                      *m_newshost;
  char                      *m_headers;
  PRInt32                   m_flags;
  PRInt32                   m_headersFP;
  bool                      m_inhead;
  PRInt32                   m_headersPosition;
  PRInt32                   m_bytesRead;
  PRInt32                   m_position;
  PRInt32                   m_flagsPosition;
  PRInt32                   m_headersSize;
  char                      *mLeftoverBuffer;
  char                      *mIdentityKey;
  char                      *mAccountKey;

  bool mSendingMessages;
  bool mUserInitiated;
  nsCOMPtr<nsIMsgIdentity> mIdentity;
};


#endif /* _nsMsgSendLater_H_ */
