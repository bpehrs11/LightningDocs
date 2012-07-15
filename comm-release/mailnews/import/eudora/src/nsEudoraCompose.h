/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 *
 * ***** BEGIN LICENSE BLOCK *****
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
 * The Original Code is mozilla.org Code.
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

#ifndef nsEudoraCompose_h__
#define nsEudoraCompose_h__

#include "nscore.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsMsgUtils.h"
#include "nsIFile.h"
#include "nsIInputStream.h"
#include "nsVoidArray.h"
#include "nsIImportService.h"

#ifdef MOZILLA_INTERNAL_API
#include "nsNativeCharsetUtils.h"
#else
#include "nsMsgI18N.h"
#define NS_CopyNativeToUnicode(source, dest) \
        nsMsgI18NConvertToUnicode(nsMsgI18NFileSystemCharset(), source, dest)
#endif

class nsIMsgSend;
class nsIMsgCompFields;
class nsIMsgIdentity;
class nsIMsgSendListener;
class nsIIOService;

#include "nsIMsgSend.h"


typedef class {
public:
  nsCOMPtr <nsIFile>  pAttachment;
  char *      mimeType;
  char *      description;
} ImportAttachment;

typedef class {
public:
  PRUint32    offset;
  PRInt64    size;
  nsCOMPtr <nsIFile>  pFile;
        nsCOMPtr <nsIInputStream> pInputStream;
} ReadFileState;

class SimpleBufferTonyRCopiedOnce {
public:
  SimpleBufferTonyRCopiedOnce() {m_pBuffer = nsnull; m_size = 0; m_growBy = 4096; m_writeOffset = 0;
          m_bytesInBuf = 0; m_convertCRs = false;}
  ~SimpleBufferTonyRCopiedOnce() { if (m_pBuffer) delete [] m_pBuffer;}

  bool Allocate( PRInt32 sz) {
    if (m_pBuffer) delete [] m_pBuffer;
    m_pBuffer = new char[sz];
    if (m_pBuffer) { m_size = sz; return( true); }
    else { m_size = 0; return( false);}
  }

  bool Grow( PRInt32 newSize) { if (newSize > m_size) return( ReAllocate( newSize)); else return( true);}
  bool ReAllocate( PRInt32 newSize) {
    if (newSize <= m_size) return( true);
    char *pOldBuffer = m_pBuffer;
    PRInt32  oldSize = m_size;
    m_pBuffer = nsnull;
    while (m_size < newSize) m_size += m_growBy;
    if (Allocate( m_size)) {
      if (pOldBuffer) { memcpy( m_pBuffer, pOldBuffer, oldSize); delete [] pOldBuffer;}
      return( true);
    }
    else { m_pBuffer = pOldBuffer; m_size = oldSize; return( false);}
  }

  bool Write( PRInt32 offset, const char *pData, PRInt32 len, PRInt32 *pWritten) {
    *pWritten = len;
    if (!len) return( true);
    if (!Grow( offset + len)) return( false);
    if (m_convertCRs)
      return( SpecialMemCpy( offset, pData, len, pWritten));
    memcpy( m_pBuffer + offset, pData, len);
    return( true);
  }

  bool Write( const char *pData, PRInt32 len) {
    PRInt32 written;
    if (Write( m_writeOffset, pData, len, &written)) { m_writeOffset += written; return( true);}
    else return( false);
  }

  bool    SpecialMemCpy( PRInt32 offset, const char *pData, PRInt32 len, PRInt32 *pWritten);

  bool    m_convertCRs;
  char *  m_pBuffer;
  PRUint32  m_bytesInBuf;  // used when reading into this buffer
  PRInt32  m_size;      // allocated size of buffer
  PRInt32  m_growBy;    // duh
  PRUint32 m_writeOffset;  // used when writing into and reading from the buffer
};



class nsEudoraCompose {
public:
  nsEudoraCompose();
  ~nsEudoraCompose();

  nsresult  SendTheMessage(nsIFile *pMailImportLocation, nsIFile **pMsg);

  void    SetBody( const char *pBody, PRInt32 len, nsCString &bodyType) { m_pBody = pBody; m_bodyLen = len; m_bodyType = bodyType;}
  void    SetHeaders( const char *pHeaders, PRInt32 len) { m_pHeaders = pHeaders; m_headerLen = len;}
  void    SetAttachments( nsVoidArray *pAttachments) { m_pAttachments = pAttachments;}
  void    SetDefaultDate( nsCString date) { m_defaultDate = date;}

  nsresult  CopyComposedMessage( nsCString& fromLine, nsIFile *pSrc, nsIOutputStream *pDst, SimpleBufferTonyRCopiedOnce& copy);

  static nsresult  FillMailBuffer( ReadFileState *pState, SimpleBufferTonyRCopiedOnce& read);
  static nsresult CreateIdentity(void);
  static void    ReleaseIdentity(void);

private:
  nsresult  CreateComponents( void);

  void    GetNthHeader( const char *pData, PRInt32 dataLen, PRInt32 n, nsCString& header, nsCString& val, bool unwrap);
  void    GetHeaderValue( const char *pData, PRInt32 dataLen, const char *pHeader, nsCString& val, bool unwrap = true);
  void    GetHeaderValue( const char *pData, PRInt32 dataLen, const char *pHeader, nsString& val) {
    val.Truncate();
    nsCString  hVal;
    GetHeaderValue( pData, dataLen, pHeader, hVal, true);
    NS_CopyNativeToUnicode( hVal, val);
  }
  void    ExtractCharset( nsString& str);
  void    ExtractType( nsString& str);

  nsresult GetLocalAttachments(nsIArray **aArray);

  nsresult  ReadHeaders( ReadFileState *pState, SimpleBufferTonyRCopiedOnce& copy, SimpleBufferTonyRCopiedOnce& header);
  PRInt32    FindNextEndLine( SimpleBufferTonyRCopiedOnce& data);
  PRInt32    IsEndHeaders( SimpleBufferTonyRCopiedOnce& data);
  PRInt32    IsSpecialHeader( const char *pHeader);
  nsresult  WriteHeaders( nsIOutputStream *pDst, SimpleBufferTonyRCopiedOnce& newHeaders);
  bool      IsReplaceHeader( const char *pHeader);

private:
  static nsIMsgIdentity *    s_pIdentity;

  nsVoidArray *      m_pAttachments;
  nsIMsgSendListener *  m_pListener;
  nsIMsgCompFields *    m_pMsgFields;
  nsCOMPtr<nsIIOService> m_pIOService;
  PRInt32          m_headerLen;
  const char *      m_pHeaders;
  PRInt32          m_bodyLen;
  const char *      m_pBody;
  nsCString        m_bodyType;
  nsString        m_defCharset;
  SimpleBufferTonyRCopiedOnce      m_readHeaders;
  nsCOMPtr<nsIImportService>  m_pImportService;
  nsCString       m_defaultDate;  // Use this if no Date: header in msgs
};


#endif /* nsEudoraCompose_h__ */
