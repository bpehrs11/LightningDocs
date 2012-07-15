/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
#ifndef _nsMimeHtmlEmitter_h_
#define _nsMimeHtmlEmitter_h_

#include "prtypes.h"
#include "prio.h"
#include "nsMimeBaseEmitter.h"
#include "nsMimeRebuffer.h"
#include "nsIStreamListener.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsIURI.h"
#include "nsIChannel.h"
#include "nsIMimeMiscStatus.h"
#include "nsIMimeConverter.h"

class nsMimeHtmlDisplayEmitter : public nsMimeBaseEmitter {
public:
    nsMimeHtmlDisplayEmitter ();
    nsresult Init();

    virtual       ~nsMimeHtmlDisplayEmitter (void);

    // Header handling routines.
    NS_IMETHOD    EndHeader(const nsACString &name);

    // Attachment handling routines
    NS_IMETHOD    StartAttachment(const nsACString &name,
                                  const char *contentType, const char *url,
                                  bool aIsExternalAttachment);
    NS_IMETHOD    AddAttachmentField(const char *field, const char *value);
    NS_IMETHOD    EndAttachment();
    NS_IMETHOD    EndAllAttachments();

    // Body handling routines
    NS_IMETHOD    WriteBody(const nsACString &buf, PRUint32 *amountWritten);
    NS_IMETHOD    EndBody();
    NS_IMETHOD    WriteHTMLHeaders(const nsACString &name);

    virtual nsresult            WriteHeaderFieldHTMLPrefix(const nsACString &name);
    virtual nsresult            WriteHeaderFieldHTML(const char *field, const char *value);
    virtual nsresult            WriteHeaderFieldHTMLPostfix();

protected:
    bool          mFirst;  // Attachment flag...
    bool          mSkipAttachment;  // attachments we shouldn't show...

    nsCOMPtr<nsIMsgHeaderSink> mHeaderSink;

    nsresult GetHeaderSink(nsIMsgHeaderSink ** aHeaderSink);
    bool BroadCastHeadersAndAttachments();
    nsresult StartAttachmentInBody(const nsACString &name,
                                   const char *contentType, const char *url);

    nsresult BroadcastHeaders(nsIMsgHeaderSink * aHeaderSink, PRInt32 aHeaderMode, bool aFromNewsgroup);
};


#endif /* _nsMimeHtmlEmitter_h_ */
