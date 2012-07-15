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
#ifndef nsPop3Sink_h__
#define nsPop3Sink_h__

#include "nscore.h"
#include "nsIURL.h"
#include "nsIPop3Sink.h"
#include "nsIOutputStream.h"
#include "prmem.h"
#include "prio.h"
#include "plstr.h"
#include "prenv.h"
#include "nsIMsgFolder.h"
#include "nsAutoPtr.h"

class nsParseNewMailState;
class nsIMsgFolder;

struct partialRecord
{
  partialRecord();
  ~partialRecord();

  nsCOMPtr<nsIMsgDBHdr> m_msgDBHdr;
  nsCString m_uidl;
};

class nsPop3Sink : public nsIPop3Sink
{
public:
    nsPop3Sink();
    virtual ~nsPop3Sink();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIPOP3SINK
    nsresult GetServerFolder(nsIMsgFolder **aFolder);
    nsresult FindPartialMessages();
    void CheckPartialMessages(nsIPop3Protocol *protocol);

    static char*  GetDummyEnvelope(void);

protected:

    nsresult WriteLineToMailbox(const char *buffer);
    nsresult ReleaseFolderLock();
    nsresult HandleTempDownloadFailed(nsIMsgWindow *msgWindow);

    bool m_authed;
    char* m_accountUrl;
    PRUint32 m_biffState;
    PRInt32 m_numNewMessages;
    PRInt32 m_numNewMessagesInFolder;
    PRInt32 m_numMsgsDownloaded;
    bool m_senderAuthed;
    char* m_outputBuffer;
    PRInt32 m_outputBufferSize;
    nsIPop3IncomingServer *m_popServer;
    //Currently the folder we want to update about biff info
    nsCOMPtr<nsIMsgFolder> m_folder;
    nsRefPtr<nsParseNewMailState> m_newMailParser;
    nsCOMPtr <nsIOutputStream> m_outFileStream; // the file we write to, which may be temporary
    nsCOMPtr<nsIMsgPluggableStore> m_msgStore;
    nsCOMPtr <nsIOutputStream> m_inboxOutputStream; // the actual mailbox
    bool m_uidlDownload;
    bool m_buildMessageUri;
    bool m_downloadingToTempFile;
    nsCOMPtr <nsILocalFile> m_tmpDownloadFile;
    nsCOMPtr<nsIMsgWindow> m_window;
    nsCString m_messageUri;
    nsCString m_baseMessageUri;
    nsCString m_origMessageUri;
    nsCString m_accountKey;
    nsTArray<partialRecord*> m_partialMsgsArray;
};

#endif
