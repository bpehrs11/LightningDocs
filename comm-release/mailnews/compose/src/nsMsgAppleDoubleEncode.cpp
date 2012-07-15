/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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

/*
*
*   apple-double.c
*	--------------
*
*  	  The codes to do apple double encoding/decoding.
*		
*		02aug95		mym		created.
*		
*/
#include "nsID.h"
#include "nscore.h"
#include "nsStringGlue.h"
#include "nsMsgAppleDouble.h"
#include "nsMsgAppleCodes.h"
#include "nsMsgCompUtils.h"
#include "nsCExternalHandlerService.h"
#include "nsIMIMEService.h"
#include "nsMimeTypes.h"
#include "prmem.h"
#include "nsNetUtil.h"


void	
MacGetFileType(nsILocalFile   *fs, 
               bool         *useDefault, 
               char         **fileType, 
               char         **encoding)
{
	if ((fs == NULL) || (fileType == NULL) || (encoding == NULL))
		return;

  bool exists = false;
  fs->Exists(&exists);
  if (!exists)
    return;

	*useDefault = TRUE;
	*fileType = NULL;
	*encoding = NULL;

  nsCOMPtr<nsILocalFileMac> macFile = do_QueryInterface(fs);
  FSRef fsRef;
  FSCatalogInfo catalogInfo;
  OSErr err = errFileOpen;
  if (NS_SUCCEEDED(macFile->GetFSRef(&fsRef)))
    err = ::FSGetCatalogInfo(&fsRef, kFSCatInfoFinderInfo, &catalogInfo, nsnull, nsnull, nsnull);

  if ( (err != noErr) || (((FileInfo*)(&catalogInfo.finderInfo))->fileType == 'TEXT') )
    *fileType = strdup(APPLICATION_OCTET_STREAM);
  else
  {
    // At this point, we should call the mime service and
    // see what we can find out?
    nsresult      rv;
    nsCOMPtr <nsIURI> tURI;
    if (NS_SUCCEEDED(NS_NewFileURI(getter_AddRefs(tURI), fs)) && tURI)
    {
      nsCOMPtr<nsIMIMEService> mimeFinder (do_GetService(NS_MIMESERVICE_CONTRACTID, &rv));
      if (NS_SUCCEEDED(rv) && mimeFinder) 
      {
        nsCAutoString mimeType;
        rv = mimeFinder->GetTypeFromURI(tURI, mimeType);
        if (NS_SUCCEEDED(rv)) 
        {
          *fileType = ToNewCString(mimeType);
          return;
        }        
      }
    }

    // If we hit here, return something...default to this...
    *fileType = strdup(APPLICATION_OCTET_STREAM);
  }
}

#pragma cplusplus reset

/*
*	ap_encode_init
*	--------------
*	
*	Setup the encode envirment
*/

int ap_encode_init( appledouble_encode_object *p_ap_encode_obj,
                    const char                *fname,
                    char                      *separator)
{
  nsCOMPtr <nsILocalFile> myFile;
  NS_NewNativeLocalFile(nsDependentCString(fname), true, getter_AddRefs(myFile));
  bool exists;
  if (myFile && NS_SUCCEEDED(myFile->Exists(&exists)) && !exists)
    return -1;

  nsCOMPtr<nsILocalFileMac> macFile = do_QueryInterface(myFile);
  nsCAutoString path;
  macFile->GetNativePath(path);

	memset(p_ap_encode_obj, 0, sizeof(appledouble_encode_object));
	
	/*
	**	Fill out the source file inforamtion.
	*/	
  memcpy(p_ap_encode_obj->fname, path.get(), path.Length());
  p_ap_encode_obj->fname[path.Length()] = '\0';
	
	p_ap_encode_obj->boundary = strdup(separator);
	return noErr;
}

/*
**	ap_encode_next
**	--------------
**		
**		return :
**			noErr	:	everything is ok
**			errDone	:	when encoding is done.
**			errors	:	otherwise.
*/
int ap_encode_next(
	appledouble_encode_object* p_ap_encode_obj, 
	char 	*to_buff, 
	PRInt32 	buff_size, 
	PRInt32* 	real_size)
{
	int status;
	
	/*
	** 	install the out buff now.
	*/
	p_ap_encode_obj->outbuff     = to_buff;
	p_ap_encode_obj->s_outbuff 	 = buff_size;
	p_ap_encode_obj->pos_outbuff = 0;
	
	/*
	**	first copy the outstandind data in the overflow buff to the out buffer. 
	*/
	if (p_ap_encode_obj->s_overflow)
	{
		status = write_stream(p_ap_encode_obj, 
								p_ap_encode_obj->b_overflow,
								p_ap_encode_obj->s_overflow);
		if (status != noErr)
			return status;
				
		p_ap_encode_obj->s_overflow = 0;
	}

	/*
	** go the next processing stage based on the current state. 
	*/
	switch (p_ap_encode_obj->state)
	{
		case kInit:
			/*
			** We are in the  starting position, fill out the header.
			*/
			status = fill_apple_mime_header(p_ap_encode_obj); 
			if (status != noErr)
				break;					/* some error happens */
				
			p_ap_encode_obj->state = kDoingHeaderPortion;
			status = ap_encode_header(p_ap_encode_obj, true); 
										/* it is the first time to calling 		*/							
			if (status == errDone)
			{
				p_ap_encode_obj->state = kDoneHeaderPortion;
			}
			else
			{
				break;					/* we need more work on header portion.	*/
			}			
				
			/*
			** we are done with the header, so let's go to the data port.
			*/
			p_ap_encode_obj->state = kDoingDataPortion;
			status = ap_encode_data(p_ap_encode_obj, true);		 	
										/* it is first time call do data portion */
							
			if (status == errDone)
			{
				p_ap_encode_obj->state  = kDoneDataPortion;
				status = noErr;
			}
			break;

		case kDoingHeaderPortion:
		
			status = ap_encode_header(p_ap_encode_obj, false); 			
										/* continue with the header portion.	*/
			if (status == errDone)
			{
				p_ap_encode_obj->state = kDoneHeaderPortion;
			}
			else
			{
				break;					/* we need more work on header portion.	*/				
			}
			
			/*
			** start the data portion.
			*/
			p_ap_encode_obj->state = kDoingDataPortion;
			status = ap_encode_data(p_ap_encode_obj, true); 					
										/* it is the first time calling 		*/
			if (status == errDone)
			{
				p_ap_encode_obj->state  = kDoneDataPortion;
				status = noErr;
			}
			break;

		case kDoingDataPortion:
		
			status = ap_encode_data(p_ap_encode_obj, false); 				
										/* it is not the first time				*/
													
			if (status == errDone)
			{
				p_ap_encode_obj->state = kDoneDataPortion;
				status = noErr;
			}
			break;

		case kDoneDataPortion:
				status = errDone;		/* we are really done.					*/

			break;
	}
	
	*real_size = p_ap_encode_obj->pos_outbuff;
	return status;
}

/*
**	ap_encode_end
**	-------------
**
**	clear the apple encoding.
*/

int ap_encode_end(
	appledouble_encode_object *p_ap_encode_obj, 
	bool is_aborting)
{
	/*
	** clear up the apple doubler.
	*/
	if (p_ap_encode_obj == NULL)
		return noErr;

	if (p_ap_encode_obj->fileId)			/* close the file if it is open.	*/
    ::FSCloseFork(p_ap_encode_obj->fileId);

	PR_FREEIF(p_ap_encode_obj->boundary);		/* the boundary string.				*/
	
	return noErr;
}
