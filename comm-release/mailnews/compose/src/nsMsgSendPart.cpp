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
 *   Pierre Phaneuf <pp@ludusdesign.com>
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
#include "nsMsgLocalFolderHdrs.h"
#include "nsMsgSend.h"
#include "nsMsgSendPart.h"
#include "nsIMimeConverter.h"
#include "nsCOMPtr.h"
#include "nsIComponentManager.h"
#include "nsMsgEncoders.h"
#include "nsMsgI18N.h"
#include "nsMsgCompUtils.h"
#include "nsMsgMimeCID.h"
#include "nsMimeTypes.h"
#include "prmem.h"
#include "nsMsgPrompts.h"
#include "nsNativeCharsetUtils.h"
#include "nsNetUtil.h"
#include "nsISeekableStream.h"
#include "nsReadLine.h"
#include "nsILineInputStream.h"
#include "nsComposeStrings.h"

static char *mime_mailto_stream_read_buffer = 0;

PRInt32 nsMsgSendPart::M_counter = 0;

nsMsgSendPart::nsMsgSendPart(nsIMsgSend* state, const char *part_charset)
{
  PL_strncpy(m_charset_name, (part_charset ? part_charset : "us-ascii"), sizeof(m_charset_name)-1);
  m_charset_name[sizeof(m_charset_name)-1] = '\0';
  m_children = nsnull;
  m_numchildren = 0;
  // if we're not added as a child, the default part number will be "1".
  m_partNum = "1";
  SetMimeDeliveryState(state);

  m_parent = nsnull;
  m_buffer = nsnull;
  m_type = nsnull;
  m_other = nsnull;
  m_strip_sensitive_headers = false;
  m_encoder_data = nsnull;
  
  m_firstBlock = false;
  m_needIntlConversion = false;
  
  m_mainpart = false;
  m_just_hit_CR = false;
}


nsMsgSendPart::~nsMsgSendPart()
{
  if (m_encoder_data) {
    MIME_EncoderDestroy(m_encoder_data, false);
    m_encoder_data = nsnull;
  }
  for (int i=0 ; i < m_numchildren; i++)
    delete m_children[i];

  delete [] m_children;
    PR_FREEIF(m_buffer);
  PR_FREEIF(m_other);
  PR_FREEIF(m_type);
}

int nsMsgSendPart::CopyString(char** dest, const char* src)
{
  NS_ASSERTION(src, "src null");
  
  PR_FREEIF(*dest);
  if (!src)
    *dest = PL_strdup("");
  else
    *dest = PL_strdup(src);
  
  return *dest? 0 : NS_ERROR_OUT_OF_MEMORY;
}


nsresult nsMsgSendPart::SetFile(nsILocalFile *file)
{
  m_file = file;
  return NS_OK;
}


int nsMsgSendPart::SetBuffer(const char* buffer)
{
  PR_FREEIF(m_buffer);
  return CopyString(&m_buffer, buffer);
}


int nsMsgSendPart::SetType(const char* type)
{
  PR_FREEIF(m_type);
  m_type = PL_strdup(type);
  return m_type ? 0 : NS_ERROR_OUT_OF_MEMORY;
}


int nsMsgSendPart::SetOtherHeaders(const char* other)
{
  return CopyString(&m_other, other);
}

int nsMsgSendPart::SetMimeDeliveryState(nsIMsgSend *state)
{
  m_state = state;
  if (GetNumChildren() > 0)
  {
    for (int i = 0; i < GetNumChildren(); i++)
    {
      nsMsgSendPart *part = GetChild(i);
      if (part) 
        part->SetMimeDeliveryState(state);
    }
  }
  return 0;
}

int nsMsgSendPart::AppendOtherHeaders(const char* more)
{
  if (!m_other)
    return SetOtherHeaders(more);

  if (!more || !*more)
    return 0;

  char* tmp = (char *) PR_Malloc(sizeof(char) * (PL_strlen(m_other) + PL_strlen(more) + 2));
  if (!tmp)
    return NS_ERROR_OUT_OF_MEMORY;

  PL_strcpy(tmp, m_other);
  PL_strcat(tmp, more);
  PR_FREEIF(m_other);
  m_other = tmp;

  return 0;
}


int nsMsgSendPart::SetEncoderData(MimeEncoderData* data)
{
  m_encoder_data = data;
  return 0;
}

int nsMsgSendPart::SetMainPart(bool value)
{
  m_mainpart = value;
  return 0;
}

int nsMsgSendPart::AddChild(nsMsgSendPart* child)
{
  m_numchildren++;
  nsMsgSendPart** tmp = new nsMsgSendPart* [m_numchildren];
  if (tmp == nsnull) return NS_ERROR_OUT_OF_MEMORY;
  for (int i=0 ; i<m_numchildren-1 ; i++) {
    tmp[i] = m_children[i];
  }
  delete [] m_children;
  m_children = tmp;
  m_children[m_numchildren - 1] = child;
  child->m_parent = this;
  nsCString partNum(m_partNum);
  partNum.Append(".");
  partNum.AppendInt(m_numchildren);
  child->m_partNum = partNum;
  return 0;
}

nsMsgSendPart * nsMsgSendPart::DetachChild(PRInt32 whichOne)
{
  nsMsgSendPart *returnValue = nsnull;
  
  NS_ASSERTION(whichOne >= 0 && whichOne < m_numchildren, "parameter out of range");
  if (whichOne >= 0 && whichOne < m_numchildren) 
  {
    returnValue = m_children[whichOne];
    
    if (m_numchildren > 1)
    {
      nsMsgSendPart** tmp = new nsMsgSendPart* [m_numchildren-1];
      if (tmp != nsnull) 
      {
        // move all the other kids over
        for (int i=0 ; i<m_numchildren-1 ; i++) 
        {
          if (i >= whichOne)
            tmp[i] = m_children[i+1];
          else
            tmp[i] = m_children[i];
        }
        delete [] m_children;
        m_children = tmp;
        m_numchildren--;
      }
    }
    else 
    {
      delete [] m_children;
      m_children = nsnull;
      m_numchildren = 0;
    }
  }
  
  if (returnValue)
    returnValue->m_parent = nsnull;
  
  return returnValue;
}

nsMsgSendPart* nsMsgSendPart::GetChild(PRInt32 which)
{
  NS_ASSERTION(which >= 0 && which < m_numchildren, "parameter out of range");
  if (which >= 0 && which < m_numchildren) {
    return m_children[which];
  }
  return nsnull;
}



int nsMsgSendPart::PushBody(const char* buffer, PRInt32 length)
{
  int status = 0;
  const char* encoded_data = buffer;
  
  if (m_encoder_data) 
  {
    status = MIME_EncoderWrite(m_encoder_data, encoded_data, length);
  } 
  else 
  {
    // Merely translate all linebreaks to CRLF.
    const char *in = encoded_data;
    const char *end = in + length;
    char *buffer, *out;
    
    
    buffer = mime_get_stream_write_buffer();
    if (!buffer) return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ASSERTION(encoded_data != buffer, "encoded_data == buffer");
    out = buffer;
    
    for (; in < end; in++) {
      if (m_just_hit_CR) {
        m_just_hit_CR = false;
        if (*in == '\n') {
          // The last thing we wrote was a CRLF from hitting a CR.
          // So, we don't want to do anything from a following LF;
          // we want to ignore it.
          continue;
        }
      }
      if (*in == '\r' || *in == '\n') {
        /* Write out the newline. */
        *out++ = '\r';
        *out++ = '\n';
        
        status = mime_write_message_body(m_state, buffer,
          out - buffer);
        if (status < 0) return status;
        out = buffer;
        
        if (*in == '\r') {
          m_just_hit_CR = true;
        }
        
        out = buffer;
      } else {
        
      /*  Fix for bug #95985. We can't assume that all lines are shorter
      than 4096 chars (MIME_BUFFER_SIZE), so we need to test
      for this here. sfraser.
        */
        if (out - buffer >= MIME_BUFFER_SIZE)
        {
          status = mime_write_message_body(m_state, buffer, out - buffer);
          if (status < 0) return status;
          
          out = buffer;
        }
        
        *out++ = *in;
      }
    }
    
    /* Flush the last line. */
    if (out > buffer) {
      status = mime_write_message_body(m_state, buffer, out - buffer);
      if (status < 0) return status;
      out = buffer;
    }
  }
  
  if (encoded_data && encoded_data != buffer) {
    PR_Free((char *) encoded_data);
  }
  
  return status;
}


/* Partition the headers into those which apply to the message as a whole;
those which apply to the message's contents; and the Content-Type header
itself.  (This relies on the fact that all body-related headers begin with
"Content-".)

  (How many header parsers are in this program now?)
  */
static int 
divide_content_headers(const char *headers,
                        char **message_headers,
                        char **content_headers,
                        char **content_type_header)
{
    const char *tail;
    char *message_tail, *content_tail, *type_tail;
    int L = 0;
    if (headers)
      L = PL_strlen(headers);
    
    if (L == 0)
      return 0;
    
    *message_headers = (char *)PR_Malloc(L+1);
    if (!*message_headers)
      return NS_ERROR_OUT_OF_MEMORY;
    
    *content_headers = (char *)PR_Malloc(L+1);
    if (!*content_headers) {
      PR_Free(*message_headers);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    *content_type_header = (char *)PR_Malloc(L+1);
    if (!*content_type_header) {
      PR_Free(*message_headers);
      PR_Free(*content_headers);
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    message_tail = *message_headers;
    content_tail = *content_headers;
    type_tail    = *content_type_header;
    tail = headers;
    
    while (*tail)
    {
      const char *head = tail;
      char **out;
      while(true) {
      /* Loop until we reach a newline that is not followed by whitespace.
        */
        if (tail[0] == 0 ||
          ((tail[0] == '\r' || tail[0] == '\n') &&
          !(tail[1] == ' ' || tail[1] == '\t' || tail[1] == '\n')))
        {
          /* Swallow the whole newline. */
          if (tail[0] == '\r' && tail[1] == '\n')
            tail++;
          if (*tail)
            tail++;
          break;
        }
        tail++;
      }
      
      /* Decide which block this header goes into.
      */
      if (!PL_strncasecmp(head, "Content-Type:", 13))
        out = &type_tail;
      else
        if (!PL_strncasecmp(head, "Content-", 8))
          out = &content_tail;
        else
          out = &message_tail;
        
        memcpy(*out, head, (tail-head));
        *out += (tail-head);
    }
    
    *message_tail = 0;
    *content_tail = 0;
    *type_tail = 0;
    
    if (!**message_headers) {
      PR_Free(*message_headers);
      *message_headers = 0;
    }
    
    if (!**content_headers) {
      PR_Free(*content_headers);
      *content_headers = 0;
    }
    
    if (!**content_type_header) {
      PR_Free(*content_type_header);
      *content_type_header = 0;
    }
    
#ifdef DEBUG
    // ### mwelch Because of the extreme difficulty we've had with
    //      duplicate part headers, I'm going to put in an
    //      ASSERT here which makes sure that no duplicate
    //      Content-Type or Content-Transfer-Encoding headers
    //      leave here undetected.
    const char* tmp;
    if (*content_type_header) {
      tmp = PL_strstr(*content_type_header, "Content-Type");
      if (tmp) {
        tmp++; // get past the first occurrence
        NS_ASSERTION(!PL_strstr(tmp, "Content-Type"), "Content-part already present");
      }
    }
    
    if (*content_headers) {
      tmp = PL_strstr(*content_headers, "Content-Transfer-Encoding");
      if (tmp) {
        tmp++; // get past the first occurrence
        NS_ASSERTION(!PL_strstr(tmp, "Content-Transfer-Encoding"), "Content-Transfert already present");
      }
    }
#endif // DEBUG
    
    return 0;
}

#define     SKIP_EMPTY_PART   1966

int 
nsMsgSendPart::Write()
{
  int     status = 0;
  char    *separator = nsnull;
  bool    needToWriteCRLFAfterEncodedBody  = false;

#define PUSHLEN(str, length)                  \
  do {                            \
    status = mime_write_message_body(m_state, str, length); \
    if (status < 0) goto FAIL;                \
  } while (0)                         \

#define PUSH(str) PUSHLEN(str, PL_strlen(str))

  // rhp: Suppress the output of parts that are empty!
  if ( (m_parent) &&
       (m_numchildren == 0) &&
       ( (!m_buffer) || (!*m_buffer) ) &&
       (!m_file) &&
       (!m_mainpart) )
    return SKIP_EMPTY_PART;

  if (m_mainpart && m_type && PL_strcmp(m_type, TEXT_HTML) == 0) 
  {     
    if (m_file) 
    {
      // The "insert HTML links" code requires a memory buffer,
      // so read the file into memory.
      NS_ASSERTION(m_buffer == nsnull, "not-null buffer");
      PRInt32           length = 0;
      PRInt64 fileSize;
      if (NS_SUCCEEDED(m_file->GetFileSize(&fileSize)))
          length = fileSize;
      
      m_buffer = (char *) PR_Malloc(sizeof(char) * (length + 1));
      if (m_buffer) 
      {
        nsCOMPtr<nsIInputStream> inputFile;
        nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(inputFile), m_file);
        if (NS_SUCCEEDED(rv)) 
        {
          PRUint32 bytesRead;
          rv = inputFile->Read(m_buffer, length, &bytesRead);
          inputFile->Close();
          m_buffer[length] = '\0';
        }
        else 
          PR_Free(m_buffer);
      }
    }
  }
  
  if (m_parent && m_parent->m_type &&
        !PL_strcasecmp(m_parent->m_type, MULTIPART_DIGEST) &&
        m_type &&
        (!PL_strcasecmp(m_type, MESSAGE_RFC822) ||
        !PL_strcasecmp(m_type, MESSAGE_NEWS))) 
  {
    // If we're in a multipart/digest, and this document is of type
    // message/rfc822, then it's appropriate to emit no headers.
    //
  }
  else 
  {
    char *message_headers = 0;
    char *content_headers = 0;
    char *content_type_header = 0;
    status = divide_content_headers(m_other,
                                    &message_headers,
                                    &content_headers,
                                    &content_type_header);
    if (status < 0)
      goto FAIL;
    
      /* First, write out all of the headers that refer to the message
      itself (From, Subject, MIME-Version, etc.)
    */
    if (message_headers) 
    {
      PUSH(message_headers);
      PR_Free(message_headers);
      message_headers = 0;
    }

    /* Now allow the crypto library to (potentially) insert some text
       (it may want to wrap the body in an envelope.)           */
    if (!m_parent) {
      status = m_state->BeginCryptoEncapsulation();
      if (status < 0) goto FAIL;
    }
          
    /* Now make sure there's a Content-Type header.
    */
    if (!content_type_header) 
    {
      NS_ASSERTION(m_type && *m_type, "null ptr");
      bool needsCharset = mime_type_needs_charset(m_type ? m_type : TEXT_PLAIN);
      if (needsCharset) 
      {
        content_type_header = PR_smprintf("Content-Type: %s; charset=%s" CRLF,
                                          (m_type ? m_type : TEXT_PLAIN), m_charset_name);
      }
      else
        content_type_header = PR_smprintf("Content-Type: %s" CRLF,
                                          (m_type ? m_type : TEXT_PLAIN));
      if (!content_type_header) 
      {
        if (content_headers)
          PR_Free(content_headers);
        status = NS_ERROR_OUT_OF_MEMORY;
        goto FAIL;
      }
    }
    
    /* If this is a compound object, tack a boundary string onto the
    Content-Type header. this
    */
    if (m_numchildren > 0)
    {
      int L;
      char *ct2;
      NS_ASSERTION(m_type, "null ptr");

      if (!separator)
      {
        separator = mime_make_separator("");
        if (!separator)
        {
          status = NS_ERROR_OUT_OF_MEMORY;
          goto FAIL;
        }
      }

      L = PL_strlen(content_type_header);
      
      if (content_type_header[L-1] == '\n')
        content_type_header[--L] = 0;
      if (content_type_header[L-1] == '\r')
        content_type_header[--L] = 0;
      
      ct2 = PR_smprintf("%s;\r\n boundary=\"%s\"" CRLF, content_type_header, separator);
      PR_Free(content_type_header);
      if (!ct2) 
      {
        if (content_headers)
          PR_Free(content_headers);
        status = NS_ERROR_OUT_OF_MEMORY;
        goto FAIL;
      }
      
      content_type_header = ct2;
    }
    
    // Now write out the Content-Type header...
    NS_ASSERTION(content_type_header && *content_type_header, "null ptr");
    PUSH(content_type_header);
    PR_Free(content_type_header);
    content_type_header = 0;
    
    /* ...followed by all of the other headers that refer to the body of
    the message (Content-Transfer-Encoding, Content-Dispositon, etc.)
    */
    if (content_headers) 
    {
      PUSH(content_headers);
      PR_Free(content_headers);
      content_headers = 0;
    }
  }

  PUSH(CRLF);         // A blank line, to mark the end of headers.

  m_firstBlock = true;
  /* only convert if we need to tag charset */
  m_needIntlConversion = mime_type_needs_charset(m_type);
  
  if (m_buffer) 
  {
    status = PushBody(m_buffer, PL_strlen(m_buffer));
    if (status < 0)
      goto FAIL;
  }
  else if (m_file) 
  {
    nsCOMPtr<nsIInputStream> inputStream;
    nsresult rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), m_file);
    if (NS_FAILED(rv))
    {
      // mysteriously disappearing?
      nsCOMPtr<nsIMsgSendReport> sendReport;
      m_state->GetSendReport(getter_AddRefs(sendReport));
      if (sendReport)
      {
        nsAutoString error_msg;
        nsMsgBuildMessageWithTmpFile(m_file, error_msg);
        sendReport->SetMessage(nsIMsgSendReport::process_Current, error_msg.get(), false);
      }
      status = NS_MSG_UNABLE_TO_OPEN_TMP_FILE;
      goto FAIL;
    }

    nsCString curLine;
    bool more = true;

    /* Kludge to avoid having to allocate memory on the toy computers... */
    if (!mime_mailto_stream_read_buffer) 
    {
      mime_mailto_stream_read_buffer = (char *) PR_Malloc(MIME_BUFFER_SIZE);
      if (!mime_mailto_stream_read_buffer) 
      {
        status = NS_ERROR_OUT_OF_MEMORY;
        goto FAIL;
      }
    }

    char    *buffer = mime_mailto_stream_read_buffer;
    if (m_strip_sensitive_headers) 
    {
      // We are attaching a message, so we should be careful to
      // strip out certain sensitive internal header fields.
      bool skipping = false;
      nsLineBuffer<char> *lineBuffer;
      
      rv = NS_InitLineBuffer(&lineBuffer);
      NS_ENSURE_SUCCESS(rv, rv);

      while (more) 
      {
        // NS_ReadLine doesn't return line termination chars.
        rv = NS_ReadLine(inputStream.get(), lineBuffer, curLine, &more);
      
        curLine.Append(CRLF);
        
        char *line = (char *) curLine.get();
        if (skipping) {
          if (*line == ' ' || *line == '\t')
            continue;
          else
            skipping = false;
        }
                
        if (!PL_strncasecmp(line, "From -", 6) ||
            !PL_strncasecmp(line, "BCC:", 4) ||
            !PL_strncasecmp(line, "FCC:", 4) ||
            !PL_strncasecmp(line, CONTENT_LENGTH ":", CONTENT_LENGTH_LEN+1) ||
            !PL_strncasecmp(line, "Lines:", 6) ||
            !PL_strncasecmp(line, "Status:", 7) ||
            !PL_strncasecmp(line, X_MOZILLA_STATUS ":", X_MOZILLA_STATUS_LEN+1) ||
            !PL_strncasecmp(line, X_MOZILLA_STATUS2 ":", X_MOZILLA_STATUS2_LEN+1) ||
            !PL_strncasecmp(line, X_MOZILLA_DRAFT_INFO ":", X_MOZILLA_DRAFT_INFO_LEN+1) ||
            !PL_strncasecmp(line, X_MOZILLA_NEWSHOST ":", X_MOZILLA_NEWSHOST_LEN+1) ||
            !PL_strncasecmp(line, X_UIDL ":", X_UIDL_LEN+1) ||
            !PL_strncasecmp(line, "X-VM-", 5)) /* hi Kyle */
        {
          skipping = true;
          continue;
        }
        
        PUSH(line);
        
        if (curLine.Length() == 2) {
          nsCOMPtr <nsISeekableStream> seekableStream = do_QueryInterface(inputStream);
          // seek back the amount of data left in the line buffer...
          seekableStream->Seek(nsISeekableStream::NS_SEEK_CUR, lineBuffer->start - lineBuffer->end);
          break;  // Now can do normal reads for the body.
        }
      }
      PR_Free(lineBuffer);
    }

    while (status >= 0) 
    {
      PRUint32 bytesRead;
      nsresult rv = inputStream->Read(buffer, MIME_BUFFER_SIZE, &bytesRead);
      if (NS_FAILED(rv))
      {  
        nsCOMPtr<nsIMsgSendReport> sendReport;
        m_state->GetSendReport(getter_AddRefs(sendReport));
        if (sendReport)
        {
          nsAutoString error_msg;
          nsMsgBuildMessageWithFile(m_file, error_msg);
          sendReport->SetMessage(nsIMsgSendReport::process_Current, error_msg.get(), false);
          status = NS_MSG_UNABLE_TO_OPEN_FILE;
          goto FAIL;
        }
      }
      status = PushBody(buffer, bytesRead);
      if (status < 0)
        goto FAIL;
      if (bytesRead < MIME_BUFFER_SIZE)
        break;
    }
  }
  
  if (m_encoder_data) 
  {
    status = MIME_EncoderDestroy(m_encoder_data, false);
    m_encoder_data = nsnull;
    needToWriteCRLFAfterEncodedBody = !m_parent;
    if (status < 0)
      goto FAIL;
  }
  
  // 
  // Ok, from here we loop and drive the the output of all children 
  // for this message.
  //
  if (m_numchildren > 0) 
  {
    bool    writeSeparator = true;

    for (int i = 0 ; i < m_numchildren ; i ++) 
    {
      if (writeSeparator)
      {
        PUSH(CRLF);
        PUSH("--");

        PUSH(separator);
        PUSH(CRLF);
      }

      status = m_children[i]->Write();
      if (status < 0)
        goto FAIL;

      if (status == SKIP_EMPTY_PART)
        writeSeparator = false;
      else
        writeSeparator = true;
    }

    PUSH(CRLF);
    PUSH("--");
    PUSH(separator);
    PUSH("--");
    PUSH(CRLF);
  }
  else if (needToWriteCRLFAfterEncodedBody)
    PUSH(CRLF);
  
FAIL:
  PR_FREEIF(separator);
  return status;
}

