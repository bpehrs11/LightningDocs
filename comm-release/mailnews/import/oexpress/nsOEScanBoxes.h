/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef nsOEScanBoxes_h___
#define nsOEScanBoxes_h___

#include "prtypes.h"
#include "nsStringGlue.h"
#include "nsIImportModule.h"
#include "nsVoidArray.h"
#include "nsISupportsArray.h"
#include "nsIFile.h"
#include "nsIImportService.h"

class nsIInputStream;

class nsOEScanBoxes {
public:
  nsOEScanBoxes();
  ~nsOEScanBoxes();

  static bool    FindMail( nsIFile *pWhere);

  bool    GetMailboxes( nsIFile *pWhere, nsISupportsArray **pArray);


private:
  typedef struct {
    PRUint32  index;
    PRUint32  parent;
    PRInt32    child;
    PRInt32    sibling;
    PRInt32    type;
    nsString  mailName;
    nsCString  fileName;
    bool      processed; // used by entries on m_pendingChildArray list
  } MailboxEntry;

  static bool    Find50Mail( nsIFile *pWhere);

  void  Reset( void);
  bool    FindMailBoxes( nsIFile * descFile);
  bool    Find50MailBoxes( nsIFile * descFile);

  // If find mailboxes fails you can use this routine to get the raw mailbox file names
  void  ScanMailboxDir( nsIFile * srcDir);
  bool    Scan50MailboxDir( nsIFile * srcDir);

  MailboxEntry *  GetIndexEntry( PRUint32 index);
  void      AddChildEntry( MailboxEntry *pEntry, PRUint32 rootIndex);
  MailboxEntry *  NewMailboxEntry(PRUint32 id, PRUint32 parent, const char *prettyName, char *pFileName);
  void        ProcessPendingChildEntries(PRUint32 parent, PRUint32 rootIndex, nsVoidArray &childArray);
  void        RemoveProcessedChildEntries();


  bool        ReadLong( nsIInputStream * stream, PRInt32& val, PRUint32 offset);
  bool        ReadLong( nsIInputStream * stream, PRUint32& val, PRUint32 offset);
  bool        ReadString( nsIInputStream * stream, nsString& str, PRUint32 offset);
  bool        ReadString( nsIInputStream * stream, nsCString& str, PRUint32 offset);
  PRUint32     CountMailboxes( MailboxEntry *pBox);

  void       BuildMailboxList( MailboxEntry *pBox, nsIFile * root, PRInt32 depth, nsISupportsArray *pArray);
  bool         GetMailboxList( nsIFile * root, nsISupportsArray **pArray);

private:
  MailboxEntry *        m_pFirst;
  nsVoidArray          m_entryArray;
  nsVoidArray          m_pendingChildArray; // contains child folders whose parent folders have not showed up.

  nsCOMPtr<nsIImportService>  mService;
};

#endif // nsOEScanBoxes_h__
