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

#ifndef _nsMsgAttachmentHandler_H_
#define _nsMsgAttachmentHandler_H_

#include "nsIURL.h"
#include "nsIMimeConverter.h"
#include "nsMsgCompFields.h"
#include "nsIMsgStatusFeedback.h"
#include "nsIChannel.h"
#include "nsIMsgSend.h"
#include "nsIFileStreams.h"
#include "nsIStreamConverter.h"

#ifdef XP_MACOSX

#include "nsMsgAppleDouble.h"
#include "nsILocalFileMac.h"

class AppleDoubleEncodeObject
{
public:
  appledouble_encode_object   ap_encode_obj;
  char                        *buff;          // the working buff
  PRInt32                     s_buff;         // the working buff size
  nsCOMPtr <nsIOutputStream>  fileStream;    // file to hold the encoding
};

class nsILocalFileMac;
class nsIZipWriter;

/* Simple utility class that will synchronously zip any file 
   (or folder hierarchy) you give it. */
class nsSimpleZipper
{
  public:
    
    // Synchronously zips the input file/folder and writes all
    // data to the output file.
    static nsresult Zip(nsIFile *aInputFile, nsIFile *aOutputFile);
  
  private:
    
    // Recursively adds the file or folder to aZipWriter.
    static nsresult AddToZip(nsIZipWriter *aZipWriter,
                             nsIFile *aFile,
                             const nsACString &aPath);
};
#endif  // XP_MACOSX

//
// This is a class that deals with processing remote attachments. It implements
// an nsIStreamListener interface to deal with incoming data
//
class nsMsgAttachmentHandler
{
public:
  nsMsgAttachmentHandler();
  ~nsMsgAttachmentHandler();
public:
  nsresult              SnarfAttachment(nsMsgCompFields *compFields);
  int                   PickEncoding (const char *charset, nsIMsgSend* mime_delivery_state);
  nsresult              PickCharset();
  void                  AnalyzeSnarfedFile ();      // Analyze a previously-snarfed file.
                                                    // (Currently only used for plaintext
                                                    // converted from HTML.) 
  nsresult              Abort();
  nsresult              UrlExit(nsresult status, const PRUnichar* aMsg);
  
  // if there's an intermediate temp file left, takes care to remove it from disk.
  //
  // NOTE: this takes care of the mEncodedWorkingFile temp file, but not mTmpFile which seems
  // to be used by lots of other classes at the moment.
  void                  CleanupTempFile();

private:

  // use when a message (e.g. original message in a reply) is attached as a rfc822 attachment.
  nsresult              SnarfMsgAttachment(nsMsgCompFields *compFields);
  bool                  UseUUEncode_p(void);
  void                  AnalyzeDataChunk (const char *chunk, PRInt32 chunkSize);
  nsresult              LoadDataFromFile(nsILocalFile *file, nsString &sigData, bool charsetConversion); //A similar function already exist in nsMsgCompose!
#ifdef XP_MACOSX
  nsresult              ConvertToAppleEncoding(const nsCString &aFileSpecURI, 
                                               const nsCString &aFilePath, 
                                               nsILocalFileMac *aSourceFile);
  // zips this attachment and does the work to make this attachment handler handle it properly.
  nsresult ConvertToZipFile(nsILocalFileMac *aSourceFile);
  bool HasResourceFork(FSRef *fsRef);
#endif

  //
public:
  nsCOMPtr<nsIURI> mURL;
  nsCOMPtr<nsILocalFile>        mTmpFile;         // The temp file to which we save it 
  nsCOMPtr<nsIOutputStream>  mOutFile;          
  nsCOMPtr<nsIRequest> mRequest; // The live request used while fetching an attachment
  nsMsgCompFields       *mCompFields;       // Message composition fields for the sender
  bool                  m_bogus_attachment; // This is to catch problem children...
  
#ifdef XP_MACOSX
  // if we need to encode this file into for example an appledouble, or zip file,
  // this file is our working file. currently only needed on mac.
  nsCOMPtr<nsILocalFile> mEncodedWorkingFile;
#endif

  nsCString m_xMacType;      // Mac file type
  nsCString m_xMacCreator;   // Mac file creator

  bool m_done;
  nsCString m_charset;         // charset name 
  nsCString m_contentId;      // This is for mutipart/related Content-ID's
  nsCString m_type;            // The real type, once we know it.
  nsCString m_typeParam;      // Any addition parameters to add to the content-type (other than charset, macType and maccreator)
  nsCString m_overrideType;   // The type we should assume it to be
                                            // or 0, if we should get it from the
                                            // server)
  nsCString m_overrideEncoding; // Goes along with override_type 

  nsCString m_desiredType;    // The type it should be converted to. 
  nsCString m_description;     // For Content-Description header
  nsCString m_realName;       // The name for the headers, if different
                                            // from the URL. 
  nsCString m_encoding;        // The encoding, once we've decided. */
  bool                  m_already_encoded_p; // If we attach a document that is already
                                             // encoded, we just pass it through.

  bool                  m_decrypted_p;  /* S/MIME -- when attaching a message that was
                                           encrypted, it's necessary to decrypt it first
                                           (since nobody but the original recipient can
                                           read it -- if you forward it to someone in the
                                           raw, it will be useless to them.)  This flag
                                           indicates whether decryption occurred, so that
                                           libmsg can issue appropriate warnings about
                                           doing a cleartext forward of a message that was
                                           originally encrypted. */

  bool                  mDeleteFile;      // If this is true, Delete the file...its 
                                          // NOT the original file!

  bool                  mMHTMLPart;           // This is true if its an MHTML part, otherwise, false
  bool                  mPartUserOmissionOverride;  // This is true if the user send send the email without this part
  bool                  mMainBody;            // True if this is a main body.
   // true if this should be sent as a link to a file.
  bool                  mSendViaCloud;
  nsString              mHtmlAnnotation;
  nsCString             mCloudProviderKey;
  nsCString             mCloudUrl;
  PRInt32 mNodeIndex; //If this is an embedded image, this is the index of the
                      // corresponding domNode in the editor's
                      //GetEmbeddedObjects. Otherwise, it will be -1.
  //
  // Vars for analyzing file data...
  //
  PRUint32              m_size;         /* Some state used while filtering it */
  PRUint32              m_unprintable_count;
  PRUint32              m_highbit_count;
  PRUint32              m_ctl_count;
  PRUint32              m_null_count;
  PRUint8               m_have_cr, m_have_lf, m_have_crlf; 
  bool                  m_prev_char_was_cr;
  PRUint32              m_current_column;
  PRUint32              m_max_column;
  PRUint32              m_lines;
  bool                  m_file_analyzed;

  MimeEncoderData       *m_encoder_data;  /* Opaque state for base64/qp encoder. */
  nsCString             m_uri; // original uri string

  nsresult              GetMimeDeliveryState(nsIMsgSend** _retval);
  nsresult              SetMimeDeliveryState(nsIMsgSend* mime_delivery_state);
private:
  nsCOMPtr<nsIMsgSend>  m_mime_delivery_state;
  nsCOMPtr<nsIStreamConverter> m_mime_parser;
  nsCOMPtr<nsIChannel>  m_converter_channel;
};


#endif /* _nsMsgAttachmentHandler_H_ */
