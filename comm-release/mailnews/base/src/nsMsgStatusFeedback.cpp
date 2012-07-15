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
 * Portions created by the Initial Developer are Copyright (C) 1999
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

#include "msgCore.h"

#include "nsIWebProgress.h"
#include "nsIXULBrowserWindow.h"
#include "nsMsgStatusFeedback.h"
#include "nsIDocument.h"
#include "nsIDOMElement.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIChannel.h"
#include "prinrval.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsIMimeMiscStatus.h"
#include "nsIMsgWindow.h"
#include "nsMsgUtils.h"
#include "nsIMsgHdr.h"
#include "nsIMsgFolder.h"
#include "nsServiceManagerUtils.h"
#include "mozilla/Services.h"

#define MSGFEEDBACK_TIMER_INTERVAL 500

nsMsgStatusFeedback::nsMsgStatusFeedback() :
  m_lastPercent(0)
{
  LL_I2L(m_lastProgressTime, 0);

  nsresult rv;
  nsCOMPtr<nsIStringBundleService> bundleService =
    mozilla::services::GetStringBundleService();

  if (bundleService)
    bundleService->CreateBundle("chrome://messenger/locale/messenger.properties",
                                getter_AddRefs(mBundle));

  m_msgLoadedAtom = MsgGetAtom("msgLoaded");
}

nsMsgStatusFeedback::~nsMsgStatusFeedback()
{
  mBundle = nsnull;
}

NS_IMPL_THREADSAFE_ADDREF(nsMsgStatusFeedback)
NS_IMPL_THREADSAFE_RELEASE(nsMsgStatusFeedback)

NS_INTERFACE_MAP_BEGIN(nsMsgStatusFeedback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMsgStatusFeedback)
  NS_INTERFACE_MAP_ENTRY(nsIMsgStatusFeedback)
  NS_INTERFACE_MAP_ENTRY(nsIProgressEventSink) 
  NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener) 
  NS_INTERFACE_MAP_ENTRY(nsISupportsWeakReference)
NS_INTERFACE_MAP_END

//////////////////////////////////////////////////////////////////////////////////
// nsMsgStatusFeedback::nsIWebProgressListener
//////////////////////////////////////////////////////////////////////////////////

NS_IMETHODIMP
nsMsgStatusFeedback::OnProgressChange(nsIWebProgress* aWebProgress,
                                      nsIRequest* aRequest,
                                      PRInt32 aCurSelfProgress,
                                      PRInt32 aMaxSelfProgress, 
                                      PRInt32 aCurTotalProgress,
                                      PRInt32 aMaxTotalProgress)
{
  PRInt32 percentage = 0;
  if (aMaxTotalProgress > 0)
  {
    percentage =  (aCurTotalProgress * 100) / aMaxTotalProgress;
    if (percentage)
      ShowProgress(percentage);
  }

   return NS_OK;
}
      
NS_IMETHODIMP
nsMsgStatusFeedback::OnStateChange(nsIWebProgress* aWebProgress,
                                   nsIRequest* aRequest,
                                   PRUint32 aProgressStateFlags,
                                   nsresult aStatus)
{
  nsresult rv;

  NS_ENSURE_TRUE(mBundle, NS_ERROR_NULL_POINTER);
  if (aProgressStateFlags & STATE_IS_NETWORK)
  {
    if (aProgressStateFlags & STATE_START)
    {
      m_lastPercent = 0;
      StartMeteors();
      nsString loadingDocument;
      rv = mBundle->GetStringFromName(NS_LITERAL_STRING("documentLoading").get(),
                                      getter_Copies(loadingDocument));
      if (NS_SUCCEEDED(rv))
        ShowStatusString(loadingDocument);
    }
    else if (aProgressStateFlags & STATE_STOP)
    {
      // if we are loading message for display purposes, this STATE_STOP notification is 
      // the only notification we get when layout is actually done rendering the message. We need
      // to fire the appropriate msgHdrSink notification in this particular case.
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
      if (channel) 
      {
        nsCOMPtr<nsIURI> uri; 
        channel->GetURI(getter_AddRefs(uri));
        nsCOMPtr<nsIMsgMailNewsUrl> mailnewsUrl (do_QueryInterface(uri));
        if (mailnewsUrl)
        {
          // get the url type
          bool messageDisplayUrl;
          mailnewsUrl->IsUrlType(nsIMsgMailNewsUrl::eDisplay, &messageDisplayUrl);

          if (messageDisplayUrl)
          {              
            // get the header sink
            nsCOMPtr<nsIMsgWindow> msgWindow;
            mailnewsUrl->GetMsgWindow(getter_AddRefs(msgWindow));
            if (msgWindow)
            {
              nsCOMPtr<nsIMsgHeaderSink> hdrSink;
              msgWindow->GetMsgHeaderSink(getter_AddRefs(hdrSink));
              if (hdrSink)
                hdrSink->OnEndMsgDownload(mailnewsUrl);
            }
            // get the folder and notify that the msg has been loaded. We're 
            // using NotifyPropertyFlagChanged. To be completely consistent,
            // we'd send a similar notification that the old message was
            // unloaded.
            nsCOMPtr <nsIMsgDBHdr> msgHdr;
            nsCOMPtr <nsIMsgFolder> msgFolder;
            mailnewsUrl->GetFolder(getter_AddRefs(msgFolder));
            nsCOMPtr <nsIMsgMessageUrl> msgUrl = do_QueryInterface(mailnewsUrl);
            if (msgUrl)
            {
              // not sending this notification is not a fatal error...
              (void) msgUrl->GetMessageHeader(getter_AddRefs(msgHdr));
              if (msgFolder && msgHdr)
                msgFolder->NotifyPropertyFlagChanged(msgHdr, m_msgLoadedAtom, 0, 1);
            }
          }
        }
      }
      StopMeteors();
      nsString documentDone;
      rv = mBundle->GetStringFromName(NS_LITERAL_STRING("documentDone").get(),
                                      getter_Copies(documentDone));
      if (NS_SUCCEEDED(rv))
        ShowStatusString(documentDone);
    }
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgStatusFeedback::OnLocationChange(nsIWebProgress* aWebProgress,
                                                    nsIRequest* aRequest,
                                                    nsIURI* aLocation,
                                                    PRUint32 aFlags)
{
   return NS_OK;
}

NS_IMETHODIMP 
nsMsgStatusFeedback::OnStatusChange(nsIWebProgress* aWebProgress,
                                    nsIRequest* aRequest,
                                    nsresult aStatus,
                                    const PRUnichar* aMessage)
{
    return NS_OK;
}


NS_IMETHODIMP 
nsMsgStatusFeedback::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
    return NS_OK;
}


NS_IMETHODIMP
nsMsgStatusFeedback::ShowStatusString(const nsAString& aStatus)
{
  nsCOMPtr<nsIMsgStatusFeedback> jsStatusFeedback(do_QueryReferent(mJSStatusFeedbackWeak));
  if (jsStatusFeedback)
    jsStatusFeedback->ShowStatusString(aStatus);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgStatusFeedback::SetStatusString(const nsAString& aStatus)
{
  nsCOMPtr<nsIMsgStatusFeedback> jsStatusFeedback(do_QueryReferent(mJSStatusFeedbackWeak));
  nsCOMPtr <nsIXULBrowserWindow> xulBrowserWindow = do_QueryInterface(jsStatusFeedback);
  if (xulBrowserWindow)
    xulBrowserWindow->SetJSDefaultStatus(aStatus);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgStatusFeedback::ShowProgress(PRInt32 aPercentage)
{
  // if the percentage hasn't changed...OR if we are going from 0 to 100% in one step
  // then don't bother....just fall out....
  if (aPercentage == m_lastPercent || (m_lastPercent == 0 && aPercentage >= 100))
    return NS_OK;
  
  m_lastPercent = aPercentage;

  PRInt64 nowMS;
  LL_I2L(nowMS, 0);
  if (aPercentage < 100)	// always need to do 100%
  {
    int64 minIntervalBetweenProgress;

    LL_I2L(minIntervalBetweenProgress, 250);
    int64 diffSinceLastProgress;
    LL_I2L(nowMS, PR_IntervalToMilliseconds(PR_IntervalNow()));
    LL_SUB(diffSinceLastProgress, nowMS, m_lastProgressTime); // r = a - b
    LL_SUB(diffSinceLastProgress, diffSinceLastProgress, minIntervalBetweenProgress); // r = a - b
    if (!LL_GE_ZERO(diffSinceLastProgress))
      return NS_OK;
  }

  m_lastProgressTime = nowMS;
  nsCOMPtr<nsIMsgStatusFeedback> jsStatusFeedback(do_QueryReferent(mJSStatusFeedbackWeak));
  if (jsStatusFeedback)
    jsStatusFeedback->ShowProgress(aPercentage);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgStatusFeedback::StartMeteors()
{
  nsCOMPtr<nsIMsgStatusFeedback> jsStatusFeedback(do_QueryReferent(mJSStatusFeedbackWeak));
  if (jsStatusFeedback)
    jsStatusFeedback->StartMeteors();
  return NS_OK;
}

NS_IMETHODIMP
nsMsgStatusFeedback::StopMeteors()
{
  nsCOMPtr<nsIMsgStatusFeedback> jsStatusFeedback(do_QueryReferent(mJSStatusFeedbackWeak));
  if (jsStatusFeedback)
    jsStatusFeedback->StopMeteors();
  return NS_OK;
}

NS_IMETHODIMP nsMsgStatusFeedback::SetWrappedStatusFeedback(nsIMsgStatusFeedback * aJSStatusFeedback)
{
  NS_ENSURE_ARG_POINTER(aJSStatusFeedback);
  mJSStatusFeedbackWeak = do_GetWeakReference(aJSStatusFeedback);
  return NS_OK;
}

NS_IMETHODIMP nsMsgStatusFeedback::OnProgress(nsIRequest *request, nsISupports* ctxt, 
                                          PRUint64 aProgress, PRUint64 aProgressMax)
{
  // XXX: What should the nsIWebProgress be?
  // XXX: this truncates 64-bit to 32-bit
  return OnProgressChange(nsnull, request, PRInt32(aProgress), PRInt32(aProgressMax), 
                          PRInt32(aProgress) /* current total progress */, PRInt32(aProgressMax) /* max total progress */);
}

NS_IMETHODIMP nsMsgStatusFeedback::OnStatus(nsIRequest *request, nsISupports* ctxt, 
                                            nsresult aStatus, const PRUnichar* aStatusArg)
{
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> sbs =
    mozilla::services::GetStringBundleService();
  NS_ENSURE_TRUE(sbs, NS_ERROR_UNEXPECTED);
  nsString str;
  rv = sbs->FormatStatusMessage(aStatus, aStatusArg, getter_Copies(str));
  NS_ENSURE_SUCCESS(rv, rv);
  return ShowStatusString(str);
}
