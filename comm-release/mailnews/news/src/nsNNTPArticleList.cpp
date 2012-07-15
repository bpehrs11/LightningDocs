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

#include "msgCore.h"    // precompiled header...

#include "nsCOMPtr.h"
#include "nsNNTPArticleList.h"
#include "nsIMsgFolder.h"
#include "nsAutoPtr.h"
#include "nsMsgKeyArray.h"

NS_IMPL_ISUPPORTS1(nsNNTPArticleList, nsINNTPArticleList)

nsNNTPArticleList::nsNNTPArticleList()
{
}

nsNNTPArticleList::~nsNNTPArticleList()
{
  if (m_newsDB) {
    m_newsDB->Commit(nsMsgDBCommitType::kSessionCommit);
    m_newsDB->Close(true);
    m_newsDB = nsnull;
  }

  m_newsFolder = nsnull;
}

NS_IMETHODIMP
nsNNTPArticleList::Initialize(nsIMsgNewsFolder *newsFolder)
{
    nsresult rv;
    NS_ENSURE_ARG_POINTER(newsFolder);

    m_dbIndex = 0;

    m_newsFolder = newsFolder;

    nsCOMPtr <nsIMsgFolder> folder = do_QueryInterface(m_newsFolder, &rv);
    NS_ENSURE_SUCCESS(rv,rv);

    rv = folder->GetMsgDatabase(getter_AddRefs(m_newsDB));
    NS_ENSURE_SUCCESS(rv,rv);
    if (!m_newsDB) return NS_ERROR_UNEXPECTED;

    nsRefPtr<nsMsgKeyArray> keys = new nsMsgKeyArray;
    rv = m_newsDB->ListAllKeys(keys);
    NS_ENSURE_SUCCESS(rv,rv);
    m_idsInDB.AppendElements(keys->m_keys);

    return NS_OK;
}

NS_IMETHODIMP
nsNNTPArticleList::AddArticleKey(PRInt32 key)
{
#ifdef DEBUG
  m_idsOnServer.AppendElement(key);
#endif

  if (m_dbIndex < m_idsInDB.Length())
  {
    PRInt32 idInDBToCheck = m_idsInDB[m_dbIndex];
    // if there are keys in the database that aren't in the newsgroup
    // on the server, remove them. We probably shouldn't do this if
    // we have a copy of the article offline.
    // We'll add to m_idsDeleted for now and remove the id later
    while (idInDBToCheck < key)
    {
      m_idsDeleted.AppendElement(idInDBToCheck);
      if (m_dbIndex >= m_idsInDB.Length())
        break;
      idInDBToCheck = m_idsInDB[++m_dbIndex];
    }
    if (idInDBToCheck == key)
      m_dbIndex++;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsNNTPArticleList::FinishAddingArticleKeys()
{
  // if the last n messages in the group are cancelled, they won't have gotten removed
  // so we have to go and remove them now.
  if (m_dbIndex < m_idsInDB.Length())
    m_idsDeleted.AppendElements(&m_idsInDB[m_dbIndex],
      m_idsInDB.Length() - m_dbIndex);
  
  if (m_idsDeleted.Length())
    m_newsFolder->RemoveMessages(m_idsDeleted);

#ifdef DEBUG
  // make sure none of the deleted turned up on the idsOnServer list
  for (PRUint32 i = 0; i < m_idsDeleted.Length(); i++) {
    NS_ASSERTION(m_idsOnServer.IndexOf((nsMsgKey)(m_idsDeleted[i]), 0) == nsMsgViewIndex_None, "a deleted turned up on the idsOnServer list");
  }
#endif
  return NS_OK;
}
