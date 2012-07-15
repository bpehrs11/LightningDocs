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

#ifndef __nsMsgAppCore_h
#define __nsMsgAppCore_h

#include "nscore.h"
#include "nsIMessenger.h"
#include "nsCOMPtr.h"
#include "nsITransactionManager.h"
#include "nsILocalFile.h"
#include "nsIDocShell.h"
#include "nsIStringBundle.h"
#include "nsILocalFile.h"
#include "nsWeakReference.h"
#include "nsIDOMWindow.h"

class nsMessenger : public nsIMessenger, public nsSupportsWeakReference, public nsIFolderListener
{

public:
  nsMessenger();
  virtual ~nsMessenger();

  NS_DECL_ISUPPORTS  
  NS_DECL_NSIMESSENGER
  NS_DECL_NSIFOLDERLISTENER

  nsresult Alert(const char * stringName);

  nsresult SaveAttachment(nsIFile *file, const nsACString& unescapedUrl,
                          const nsACString& messageUri, const nsACString& contentType, 
                          void *closure, nsIUrlListener *aListener);
  nsresult PromptIfFileExists(nsILocalFile *file);
  nsresult DetachAttachments(PRUint32 aCount,
                             const char ** aContentTypeArray,
                             const char ** aUrlArray,
                             const char ** aDisplayNameArray,
                             const char ** aMessageUriArray,
                             nsTArray<nsCString> *saveFileUris,
                             bool withoutWarning = false);
  nsresult SaveAllAttachments(PRUint32 count,
                              const char **contentTypeArray,
                              const char **urlArray,
                              const char **displayNameArray,
                              const char **messageUriArray,
                              bool detaching);
  nsresult SaveOneAttachment(const char* aContentType,
                             const char* aURL,
                             const char* aDisplayName,
                             const char* aMessageUri,
                             bool detaching);

protected:
  void GetString(const nsString& aStringName, nsString& stringValue);
  nsresult InitStringBundle();
  nsresult PromptIfDeleteAttachments(bool saveFirst, PRUint32 count, const char **displayNameArray);

private:
  nsresult GetLastSaveDirectory(nsILocalFile **aLastSaveAsDir);
  // if aLocalFile is a dir, we use it.  otherwise, we use the parent of aLocalFile.
  nsresult SetLastSaveDirectory(nsILocalFile *aLocalFile);

  nsresult GetSaveAsFile(const nsAString& aMsgFilename, PRInt32 *aSaveAsFileType,
                         nsILocalFile **aSaveAsFile);

  nsresult GetSaveToDir(nsILocalFile **aSaveToDir);

  nsString mId;
  nsCOMPtr<nsITransactionManager> mTxnMgr;

  /* rhp - need this to drive message display */
  nsCOMPtr<nsIDOMWindow>    mWindow;
  nsCOMPtr<nsIMsgWindow>    mMsgWindow;
  nsCOMPtr<nsIDocShell>     mDocShell;

  // String bundles...
  nsCOMPtr<nsIStringBundle>   mStringBundle;

  nsCString mCurrentDisplayCharset;

  nsCOMPtr<nsISupports>  mSearchContext;
  nsCString   mLastDisplayURI; // this used when the user attempts to force a charset reload of a message...we need to get the last displayed
                               // uri so we can re-display it..
  nsCString mNavigatingToUri;
  nsTArray<nsCString> mLoadedMsgHistory;
  PRInt32 mCurHistoryPos;
};

#define NS_MESSENGER_CID \
{ /* f436a174-e2c0-4955-9afe-e3feb68aee56 */      \
  0xf436a174, 0xe2c0, 0x4955,                     \
    {0x9a, 0xfe, 0xe3, 0xfe, 0xb6, 0x8a, 0xee, 0x56}}

#endif
