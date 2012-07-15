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

#include "nscore.h"
#include "nsStringGlue.h"
#include "prio.h"
#include "nsNetUtil.h"
#include "nsISeekableStream.h"
#include "nsMsgUtils.h"
#include "ImportOutFile.h"
#include "ImportCharSet.h"

#include "ImportDebug.h"

/*
#ifdef _MAC
#define  kMacNoCreator    '????'
#define kMacTextFile    'TEXT'
#else
#define  kMacNoCreator    0
#define kMacTextFile    0
#endif
*/

ImportOutFile::ImportOutFile()
{
  m_ownsFileAndBuffer = false;
  m_pos = 0;
  m_pBuf = nsnull;
  m_bufSz = 0;
  m_pTrans = nsnull;
  m_pTransOut = nsnull;
  m_pTransBuf = nsnull;
}

ImportOutFile::ImportOutFile( nsIFile *pFile, PRUint8 * pBuf, PRUint32 sz)
{
  m_pTransBuf = nsnull;
  m_pTransOut = nsnull;
  m_pTrans = nsnull;
  m_ownsFileAndBuffer = false;
  InitOutFile( pFile, pBuf, sz);
}

ImportOutFile::~ImportOutFile()
{
  if (m_ownsFileAndBuffer)
  {
    Flush();
    delete [] m_pBuf;
  }

  delete m_pTrans;
  delete m_pTransOut;
  delete m_pTransBuf;
}

bool ImportOutFile::Set8bitTranslator( nsImportTranslator *pTrans)
{
  if (!Flush())
    return( false);

  m_engaged = false;
  m_pTrans = pTrans;
  m_supports8to7 = pTrans->Supports8bitEncoding();


  return( true);
}

bool ImportOutFile::End8bitTranslation( bool *pEngaged, nsCString& useCharset, nsCString& encoding)
{
  if (!m_pTrans)
    return( false);


  bool bResult = Flush();
  if (m_supports8to7 && m_pTransOut) {
    if (bResult)
      bResult = m_pTrans->FinishConvertToFile( m_pTransOut);
    if (bResult)
      bResult = Flush();
  }

  if (m_supports8to7) {
    m_pTrans->GetCharset( useCharset);
    m_pTrans->GetEncoding( encoding);
  }
  else
    useCharset.Truncate();
  *pEngaged = m_engaged;
  delete m_pTrans;
  m_pTrans = nsnull;
  delete m_pTransOut;
  m_pTransOut = nsnull;
  delete m_pTransBuf;
  m_pTransBuf = nsnull;

  return( bResult);
}

bool ImportOutFile::InitOutFile( nsIFile *pFile, PRUint32 bufSz)
{
  if (!bufSz)
    bufSz = 32 * 1024;
  if (!m_pBuf) {
    m_pBuf = new PRUint8[ bufSz];
  }

  // m_fH = UFile::CreateFile( oFile, kMacNoCreator, kMacTextFile);
        if (!m_outputStream)
        {
          nsresult rv = MsgNewBufferedFileOutputStream(getter_AddRefs(m_outputStream),
                                   pFile,
                                   PR_CREATE_FILE | PR_WRONLY | PR_TRUNCATE,
                                   0644);

    if (NS_FAILED( rv))
                {
      IMPORT_LOG0( "Couldn't create outfile\n");
      delete [] m_pBuf;
      m_pBuf = nsnull;
      return( false);
    }
  }
  m_pFile = pFile;
  m_ownsFileAndBuffer = true;
  m_pos = 0;
  m_bufSz = bufSz;
  return( true);
}

void ImportOutFile::InitOutFile( nsIFile *pFile, PRUint8 * pBuf, PRUint32 sz)
{
  m_ownsFileAndBuffer = false;
  m_pFile = pFile;
  m_pBuf = pBuf;
  m_bufSz = sz;
  m_pos = 0;
}



bool ImportOutFile::Flush( void)
{
  if (!m_pos)
    return( true);

  PRUint32  transLen;
  bool      duddleyDoWrite = false;

  // handle translations if appropriate
  if (m_pTrans) {
    if (m_engaged && m_supports8to7) {
      // Markers can get confused by this crap!!!
      // TLR: FIXME: Need to update the markers based on
      // the difference between the translated len and untranslated len

      if (!m_pTrans->ConvertToFile(  m_pBuf, m_pos, m_pTransOut, &transLen))
        return( false);
      if (!m_pTransOut->Flush())
        return( false);
      // now update our buffer...
      if (transLen < m_pos) {
        memcpy( m_pBuf, m_pBuf + transLen, m_pos - transLen);
      }
      m_pos -= transLen;
    }
    else if (m_engaged) {
      // does not actually support translation!
      duddleyDoWrite = true;
    }
    else {
      // should we engage?
      PRUint8 *  pChar = m_pBuf;
      PRUint32  len = m_pos;
      while (len) {
        if (!ImportCharSet::IsUSAscii( *pChar))
          break;
        pChar++;
        len--;
      }
      if (len) {
        m_engaged = true;
        if (m_supports8to7) {
          // allocate our translation output buffer and file...
          m_pTransBuf = new PRUint8[m_bufSz];
          m_pTransOut = new ImportOutFile( m_pFile, m_pTransBuf, m_bufSz);
          return( Flush());
        }
        else
          duddleyDoWrite = true;
      }
      else {
        duddleyDoWrite = true;
      }
    }
  }
  else
    duddleyDoWrite = true;

  if (duddleyDoWrite) {
    PRUint32 written = 0;
    nsresult rv = m_outputStream->Write( (const char *)m_pBuf, (PRInt32)m_pos, &written);
    if (NS_FAILED( rv) || ((PRUint32)written != m_pos))
      return( false);
    m_pos = 0;
  }

  return( true);
}

bool ImportOutFile::WriteU8NullTerm( const PRUint8 * pSrc, bool includeNull)
{
  while (*pSrc) {
    if (m_pos >= m_bufSz) {
      if (!Flush())
        return( false);
    }
    *(m_pBuf + m_pos) = *pSrc;
    m_pos++;
    pSrc++;
  }
  if (includeNull) {
    if (m_pos >= m_bufSz) {
      if (!Flush())
        return( false);
    }
    *(m_pBuf + m_pos) = 0;
    m_pos++;
  }

  return( true);
}

bool ImportOutFile::SetMarker( int markerID)
{
  if (!Flush()) {
    return( false);
  }

  if (markerID < kMaxMarkers) {
    PRInt64 pos = 0;
    if (m_outputStream)
                {
                  // do we need to flush for the seek to give us the right pos?
                  m_outputStream->Flush();
                  nsresult rv;
                  nsCOMPtr <nsISeekableStream> seekStream = do_QueryInterface(m_outputStream, &rv);
                  NS_ENSURE_SUCCESS(rv, false);
      rv = seekStream->Tell( &pos);
      if (NS_FAILED( rv)) {
        IMPORT_LOG0( "*** Error, Tell failed on output stream\n");
        return( false);
      }
    }
    m_markers[markerID] = (PRUint32)pos + m_pos;
  }

  return( true);
}

void ImportOutFile::ClearMarker( int markerID)
{
  if (markerID < kMaxMarkers)
    m_markers[markerID] = 0;
}

bool ImportOutFile::WriteStrAtMarker( int markerID, const char *pStr)
{
  if (markerID >= kMaxMarkers)
    return( false);

  if (!Flush())
    return( false);
  PRInt64    pos;
        m_outputStream->Flush();
        nsresult rv;
        nsCOMPtr <nsISeekableStream> seekStream = do_QueryInterface(m_outputStream, &rv);
        NS_ENSURE_SUCCESS(rv, false);
  rv = seekStream->Tell( &pos);
  if (NS_FAILED( rv))
    return( false);
  rv = seekStream->Seek(nsISeekableStream::NS_SEEK_SET, (PRInt32) m_markers[markerID]);
  if (NS_FAILED( rv))
    return( false);
  PRUint32 written;
  rv = m_outputStream->Write( pStr, strlen( pStr), &written);
  if (NS_FAILED( rv))
    return( false);

  rv = seekStream->Seek(nsISeekableStream::NS_SEEK_SET, pos);
  if (NS_FAILED( rv))
    return( false);

  return( true);
}

