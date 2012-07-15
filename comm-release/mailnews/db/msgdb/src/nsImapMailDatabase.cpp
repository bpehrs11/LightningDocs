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
 * Portions created by the Initial Developer are Copyright (C) 1999-2003
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

#include <sys/stat.h>

#include "msgCore.h"
#include "nsImapMailDatabase.h"
#include "nsDBFolderInfo.h"

const char *kPendingHdrsScope = "ns:msg:db:row:scope:pending:all";	// scope for all offine ops table
const char *kPendingHdrsTableKind = "ns:msg:db:table:kind:pending";
struct mdbOid gAllPendingHdrsTableOID;

nsImapMailDatabase::nsImapMailDatabase()
{
  m_mdbAllPendingHdrsTable = nsnull;
}

nsImapMailDatabase::~nsImapMailDatabase()
{
}

NS_IMETHODIMP	nsImapMailDatabase::GetSummaryValid(bool *aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  if (m_dbFolderInfo)
  {
    PRUint32 version;
    m_dbFolderInfo->GetVersion(&version);
    *aResult = (GetCurVersion() == version);
  }
  else
      *aResult = false;

  return NS_OK;
}

NS_IMETHODIMP	nsImapMailDatabase::SetSummaryValid(bool valid)
{
  if (m_dbFolderInfo)
  {
    m_dbFolderInfo->SetVersion(valid ? GetCurVersion() : 0);
    Commit(nsMsgDBCommitType::kLargeCommit);
  }
  return NS_OK;
}

// IMAP does not set local file flags, override does nothing
void nsImapMailDatabase::UpdateFolderFlag(nsIMsgDBHdr * /* msgHdr */, bool /* bSet */,
                                          nsMsgMessageFlagType /* flag */, nsIOutputStream ** /* ppFileStream */)
{
}

// We override this to avoid our parent class (nsMailDatabase)'s 
// grabbing of the folder semaphore, and bailing on failure.
NS_IMETHODIMP nsImapMailDatabase::DeleteMessages(PRUint32 aNumKeys, nsMsgKey* nsMsgKeys, nsIDBChangeListener *instigator)
{
  return nsMsgDatabase::DeleteMessages(aNumKeys, nsMsgKeys, instigator);
}

// override so nsMailDatabase methods that deal with m_folderStream are *not* called
NS_IMETHODIMP nsImapMailDatabase::StartBatch()
{
  return NS_OK;
}

NS_IMETHODIMP nsImapMailDatabase::EndBatch()
{
  return NS_OK;
}

nsresult nsImapMailDatabase::AdjustExpungedBytesOnDelete(nsIMsgDBHdr *msgHdr)
{
  PRUint32 msgFlags;
  msgHdr->GetFlags(&msgFlags);
  if (msgFlags & nsMsgMessageFlags::Offline && m_dbFolderInfo)
  {
    PRUint32 size = 0;
    (void)msgHdr->GetOfflineMessageSize(&size);
    return m_dbFolderInfo->ChangeExpungedBytes (size);
  }
  return NS_OK;
}

NS_IMETHODIMP nsImapMailDatabase::ForceClosed()
{
  m_mdbAllPendingHdrsTable = nsnull;
  return nsMailDatabase::ForceClosed();
}

nsresult nsImapMailDatabase::GetAllPendingHdrsTable()
{
  nsresult rv = NS_OK;
  if (!m_mdbAllPendingHdrsTable)
    rv = GetTableCreateIfMissing(kPendingHdrsScope, kPendingHdrsTableKind, getter_AddRefs(m_mdbAllPendingHdrsTable),
                                                m_pendingHdrsRowScopeToken, m_pendingHdrsTableKindToken) ;
  return rv;
}

NS_IMETHODIMP nsImapMailDatabase::AddNewHdrToDB(nsIMsgDBHdr *newHdr, bool notify)
{
  nsresult rv = nsMsgDatabase::AddNewHdrToDB(newHdr, notify);
  if (NS_SUCCEEDED(rv))
    rv = UpdatePendingAttributes(newHdr);
  return rv;
}

NS_IMETHODIMP nsImapMailDatabase::UpdatePendingAttributes(nsIMsgDBHdr* aNewHdr)
{
  nsresult rv = GetAllPendingHdrsTable();
  NS_ENSURE_SUCCESS(rv, rv);
  mdb_count numPendingHdrs = 0;
  m_mdbAllPendingHdrsTable->GetCount(GetEnv(), &numPendingHdrs);
  if (numPendingHdrs > 0)
  {
    mdbYarn messageIdYarn;
    nsCOMPtr <nsIMdbRow> pendingRow;
    mdbOid  outRowId;

    nsCString messageId;
    aNewHdr->GetMessageId(getter_Copies(messageId));
    messageIdYarn.mYarn_Buf = (void*)messageId.get();
    messageIdYarn.mYarn_Fill = messageId.Length();
    messageIdYarn.mYarn_Form = 0;
    messageIdYarn.mYarn_Size = messageIdYarn.mYarn_Fill;

    m_mdbStore->FindRow(GetEnv(), m_pendingHdrsRowScopeToken,
              m_messageIdColumnToken, &messageIdYarn, &outRowId, getter_AddRefs(pendingRow));
    if (pendingRow)
    {
      mdb_count numCells;
      mdbYarn cellYarn;
      mdb_column cellColumn;
      PRUint32 existingFlags;

      pendingRow->GetCount(GetEnv(), &numCells);
      aNewHdr->GetFlags(&existingFlags);
      // iterate over the cells in the pending hdr setting properties on the aNewHdr.
      // we skip cell 0, which is the messageId;
      nsMsgHdr* msgHdr = static_cast<nsMsgHdr*>(aNewHdr);      // closed system, cast ok
      nsIMdbRow *row = msgHdr->GetMDBRow();
      for (mdb_count cellIndex = 1; cellIndex < numCells; cellIndex++)
      {
        mdb_err err = pendingRow->SeekCellYarn(GetEnv(), cellIndex, &cellColumn, nsnull);
        if (err == 0)
        {
          err = pendingRow->AliasCellYarn(GetEnv(), cellColumn, &cellYarn);
          if (err == 0)
          {
            if (row)
              row->AddColumn(GetEnv(), cellColumn, &cellYarn);
          }
        }
      }
      // We might have changed some cached values, so force a refresh.
      msgHdr->ClearCachedValues();
      PRUint32 resultFlags;
      msgHdr->OrFlags(existingFlags, &resultFlags);
      m_mdbAllPendingHdrsTable->CutRow(GetEnv(), pendingRow);
      pendingRow->CutAllColumns(GetEnv());
    }
  }
  return rv;
}

nsresult nsImapMailDatabase::GetRowForPendingHdr(nsIMsgDBHdr *pendingHdr,
                                                 nsIMdbRow **row)
{
  nsresult rv = GetAllPendingHdrsTable();
  NS_ENSURE_SUCCESS(rv, rv);

  mdbYarn messageIdYarn;
  nsCOMPtr<nsIMdbRow> pendingRow;
  mdbOid  outRowId;
  nsCString messageId;
  pendingHdr->GetMessageId(getter_Copies(messageId));
  messageIdYarn.mYarn_Buf = (void*)messageId.get();
  messageIdYarn.mYarn_Fill = messageId.Length();
  messageIdYarn.mYarn_Form = 0;
  messageIdYarn.mYarn_Size = messageIdYarn.mYarn_Fill;

  rv = m_mdbStore->FindRow(GetEnv(), m_pendingHdrsRowScopeToken,
            m_messageIdColumnToken, &messageIdYarn, &outRowId, getter_AddRefs(pendingRow));

  if (!pendingRow)
    rv  = m_mdbStore->NewRow(GetEnv(), m_pendingHdrsRowScopeToken, getter_AddRefs(pendingRow));

  NS_ENSURE_SUCCESS(rv, rv);
  if (pendingRow)
  {
    // now we need to add cells to the row to remember the messageid, property and property value, and flags.
    // Then, when hdrs are added to the db, we'll check if they have a matching message-id, and if so,
    // set the property and flags
    // XXX we already fetched messageId from the pending hdr, could it have changed by the time we get here? 
    nsCString messageId;
    pendingHdr->GetMessageId(getter_Copies(messageId));
    // we're just going to ignore messages without a message-id. They should be rare. If SPAM messages often
    // didn't have message-id's, they'd be filtered on the server, most likely, and spammers would then
    // start putting in message-id's.
    if (!messageId.IsEmpty())
    {
       extern const char *kMessageIdColumnName;
       m_mdbAllPendingHdrsTable->AddRow(GetEnv(), pendingRow);
       // make sure this is the first cell so that when we ignore the first
       // cell in nsImapMailDatabase::AddNewHdrToDB, we're ignoring the right one
      (void) SetProperty(pendingRow, kMessageIdColumnName, messageId.get());
      pendingRow.forget(row);
    }
    else
      return NS_ERROR_FAILURE;
  }
  return rv;
}

NS_IMETHODIMP nsImapMailDatabase::SetAttributeOnPendingHdr(nsIMsgDBHdr *pendingHdr, const char *property,
                                  const char *propertyVal)
{
  NS_ENSURE_ARG_POINTER(pendingHdr);
  nsCOMPtr<nsIMdbRow> pendingRow;
  nsresult rv = GetRowForPendingHdr(pendingHdr, getter_AddRefs(pendingRow));
  NS_ENSURE_SUCCESS(rv, rv);
  return SetProperty(pendingRow, property, propertyVal);
}

NS_IMETHODIMP
nsImapMailDatabase::SetUint32AttributeOnPendingHdr(nsIMsgDBHdr *pendingHdr,
                                                   const char *property,
                                                   PRUint32 propertyVal)
{
  NS_ENSURE_ARG_POINTER(pendingHdr);
  nsCOMPtr<nsIMdbRow> pendingRow;
  nsresult rv = GetRowForPendingHdr(pendingHdr, getter_AddRefs(pendingRow));
  NS_ENSURE_SUCCESS(rv, rv);
  return SetUint32Property(pendingRow, property, propertyVal);
}

NS_IMETHODIMP
nsImapMailDatabase::SetUint64AttributeOnPendingHdr(nsIMsgDBHdr *aPendingHdr,
                                                   const char *aProperty,
                                                   PRUint64 aPropertyVal)
{
  NS_ENSURE_ARG_POINTER(aPendingHdr);
  nsCOMPtr<nsIMdbRow> pendingRow;
  nsresult rv = GetRowForPendingHdr(aPendingHdr, getter_AddRefs(pendingRow));
  NS_ENSURE_SUCCESS(rv, rv);
  return SetUint64Property(pendingRow, aProperty, aPropertyVal);
}
