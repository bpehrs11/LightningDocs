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
#ifndef ImportOutFile_h___
#define ImportOutFile_h___

#include "nsImportTranslator.h"
#include "nsIOutputStream.h"
#include "nsIFile.h"

#define kMaxMarkers    10

class ImportOutFile;

class ImportOutFile {
public:
  ImportOutFile();
  ImportOutFile( nsIFile *pFile, PRUint8 * pBuf, PRUint32 sz);
  ~ImportOutFile();

  bool    InitOutFile( nsIFile *pFile, PRUint32 bufSz = 4096);
  void  InitOutFile( nsIFile *pFile, PRUint8 * pBuf, PRUint32 sz);
  inline bool    WriteData( const PRUint8 * pSrc, PRUint32 len);
  inline bool    WriteByte( PRUint8 byte);
  bool    WriteStr( const char *pStr) {return( WriteU8NullTerm( (const PRUint8 *) pStr, false)); }
  bool    WriteU8NullTerm( const PRUint8 * pSrc, bool includeNull);
  bool    WriteEol( void) { return( WriteStr( "\x0D\x0A")); }
  bool    Done( void) {return( Flush());}

  // Marker support
  bool    SetMarker( int markerID);
  void  ClearMarker( int markerID);
  bool    WriteStrAtMarker( int markerID, const char *pStr);

  // 8-bit to 7-bit translation
  bool    Set8bitTranslator( nsImportTranslator *pTrans);
  bool    End8bitTranslation( bool *pEngaged, nsCString& useCharset, nsCString& encoding);

protected:
  bool    Flush( void);

protected:
  nsCOMPtr <nsIFile>      m_pFile;
        nsCOMPtr <nsIOutputStream> m_outputStream;
  PRUint8 *    m_pBuf;
  PRUint32    m_bufSz;
  PRUint32    m_pos;
  bool        m_ownsFileAndBuffer;

  // markers
  PRUint32    m_markers[kMaxMarkers];

  // 8 bit to 7 bit translations
  nsImportTranslator  *  m_pTrans;
  bool            m_engaged;
  bool            m_supports8to7;
  ImportOutFile *      m_pTransOut;
  PRUint8 *        m_pTransBuf;
};

inline bool    ImportOutFile::WriteData( const PRUint8 * pSrc, PRUint32 len) {
  while ((len + m_pos) > m_bufSz) {
    if ((m_bufSz - m_pos)) {
      memcpy( m_pBuf + m_pos, pSrc, m_bufSz - m_pos);
      len -= (m_bufSz - m_pos);
      pSrc += (m_bufSz - m_pos);
      m_pos = m_bufSz;
    }
    if (!Flush())
      return( false);
  }

  if (len) {
    memcpy( m_pBuf + m_pos, pSrc, len);
    m_pos += len;
  }

  return( true);
}

inline bool    ImportOutFile::WriteByte( PRUint8 byte) {
  if (m_pos == m_bufSz) {
    if (!Flush())
      return( false);
  }
  *(m_pBuf + m_pos) = byte;
  m_pos++;
  return( true);
}

#endif /* ImportOutFile_h__ */


