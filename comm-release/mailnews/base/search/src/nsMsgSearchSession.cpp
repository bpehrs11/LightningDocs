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
 * Portions created by the Initial Developer are Copyright (C) 2000
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Howard Chu <hyc@symas.com>
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

#include "msgCore.h"
#include "nsMsgSearchCore.h"
#include "nsMsgSearchAdapter.h"
#include "nsMsgSearchBoolExpression.h"
#include "nsMsgSearchSession.h"
#include "nsMsgResultElement.h"
#include "nsMsgSearchTerm.h"
#include "nsMsgSearchScopeTerm.h"
#include "nsIMsgMessageService.h"
#include "nsMsgUtils.h"
#include "nsIMsgSearchNotify.h"
#include "nsIMsgMailSession.h"
#include "nsMsgBaseCID.h"
#include "nsMsgFolderFlags.h"
#include "nsMsgLocalSearch.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsAutoPtr.h"

NS_IMPL_ISUPPORTS3(nsMsgSearchSession, nsIMsgSearchSession, nsIUrlListener, nsISupportsWeakReference)

nsMsgSearchSession::nsMsgSearchSession()
{
  m_sortAttribute = nsMsgSearchAttrib::Sender;
  m_idxRunningScope = 0;
  m_urlQueueIndex = 0;
  m_handlingError = false;
  m_expressionTree = nsnull;
  m_searchPaused = false;
  NS_NewISupportsArray(getter_AddRefs(m_termList));
}

nsMsgSearchSession::~nsMsgSearchSession()
{
  InterruptSearch();
  delete m_expressionTree;
  DestroyScopeList ();
  DestroyTermList ();
}

/* [noscript] void AddSearchTerm (in nsMsgSearchAttribute attrib, in nsMsgSearchOperator op, in nsMsgSearchValue value, in boolean BooleanAND, in string customString); */
NS_IMETHODIMP
nsMsgSearchSession::AddSearchTerm(nsMsgSearchAttribValue attrib,
                                  nsMsgSearchOpValue op,
                                  nsIMsgSearchValue * value,
                                  bool BooleanANDp,
                                  const char *customString)
{
    // stupid gcc
    nsMsgSearchBooleanOperator boolOp;
    if (BooleanANDp)
        boolOp = (nsMsgSearchBooleanOperator)nsMsgSearchBooleanOp::BooleanAND;
    else
        boolOp = (nsMsgSearchBooleanOperator)nsMsgSearchBooleanOp::BooleanOR;
  nsMsgSearchTerm *pTerm = new nsMsgSearchTerm (attrib, op, value,
                                                  boolOp, customString);
  if (nsnull == pTerm)
    return NS_ERROR_OUT_OF_MEMORY;
  m_termList->AppendElement (pTerm);
        // force the expression tree to rebuild whenever we change the terms
        delete m_expressionTree;
        m_expressionTree = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
nsMsgSearchSession::AppendTerm(nsIMsgSearchTerm *aTerm)
{
    NS_ENSURE_ARG_POINTER(aTerm);
    NS_ENSURE_TRUE(m_termList, NS_ERROR_NOT_INITIALIZED);
    delete m_expressionTree;
    m_expressionTree = nsnull;
    return m_termList->AppendElement(aTerm);
}

NS_IMETHODIMP
nsMsgSearchSession::GetSearchTerms(nsISupportsArray **aResult)
{
    NS_ENSURE_ARG_POINTER(aResult);
    *aResult = m_termList;
    NS_ADDREF(*aResult);
    return NS_OK;
}

NS_IMETHODIMP
nsMsgSearchSession::CreateTerm(nsIMsgSearchTerm **aResult)
{
    nsMsgSearchTerm *term = new nsMsgSearchTerm;
    NS_ENSURE_TRUE(term, NS_ERROR_OUT_OF_MEMORY);

    *aResult = static_cast<nsIMsgSearchTerm*>(term);
    NS_ADDREF(*aResult);
    return NS_OK;
}

/* void RegisterListener (in nsIMsgSearchNotify listener); */
NS_IMETHODIMP nsMsgSearchSession::RegisterListener(nsIMsgSearchNotify *aListener,
                                                   PRInt32 aNotifyFlags)
{
  NS_ENSURE_ARG_POINTER(aListener);
  m_listenerList.AppendElement(aListener);
  m_listenerFlagList.AppendElement(aNotifyFlags);
  return NS_OK;
}

/* void UnregisterListener (in nsIMsgSearchNotify listener); */
NS_IMETHODIMP nsMsgSearchSession::UnregisterListener(nsIMsgSearchNotify *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  PRInt32 listenerIndex = m_listenerList.IndexOf(aListener);
  if (listenerIndex != -1)
  {
    m_listenerList.RemoveElementAt(listenerIndex);
    m_listenerFlagList.RemoveElementAt(listenerIndex);

    // Adjust our iterator if it is active.
    // Removal of something at a higher index than the iterator does not affect
    // it; we only care if the the index we were pointing at gets shifted down,
    // in which case we also want to shift down.
    if (m_iListener != -1 && listenerIndex <= m_iListener)
      m_iListener--;
  }

  return NS_OK;
}

/* readonly attribute long numSearchTerms; */
NS_IMETHODIMP nsMsgSearchSession::GetNumSearchTerms(PRUint32 *aNumSearchTerms)
{
  NS_ENSURE_ARG(aNumSearchTerms);
  return m_termList->Count(aNumSearchTerms);
}

/* [noscript] void GetNthSearchTerm (in long whichTerm, in nsMsgSearchAttribute attrib, in nsMsgSearchOperator op, in nsMsgSearchValue value); */
NS_IMETHODIMP
nsMsgSearchSession::GetNthSearchTerm(PRInt32 whichTerm,
                                     nsMsgSearchAttribValue attrib,
                                     nsMsgSearchOpValue op,
                                     nsIMsgSearchValue * value)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long CountSearchScopes (); */
NS_IMETHODIMP nsMsgSearchSession::CountSearchScopes(PRInt32 *_retval)
{
  NS_ENSURE_ARG(_retval);
  *_retval = m_scopeList.Length();
  return NS_OK;
}

  /* void GetNthSearchScope (in long which, out nsMsgSearchScope scopeId, out nsIMsgFolder folder); */
NS_IMETHODIMP
nsMsgSearchSession::GetNthSearchScope(PRInt32 which,
                                      nsMsgSearchScopeValue *scopeId,
                                      nsIMsgFolder **folder)
{
  nsMsgSearchScopeTerm *scopeTerm = m_scopeList.SafeElementAt(which, nsnull);
  if (!scopeTerm)
    return NS_ERROR_INVALID_ARG;

  *scopeId = scopeTerm->m_attribute;
  *folder = scopeTerm->m_folder;
  NS_IF_ADDREF(*folder);
  return NS_OK;
}

/* void AddScopeTerm (in nsMsgSearchScopeValue scope, in nsIMsgFolder folder); */
NS_IMETHODIMP
nsMsgSearchSession::AddScopeTerm(nsMsgSearchScopeValue scope,
                                 nsIMsgFolder *folder)
{
  if (scope != nsMsgSearchScope::allSearchableGroups)
  {
    NS_ASSERTION(folder, "need folder if not searching all groups");
    if (!folder)
      return NS_ERROR_NULL_POINTER;
  }

  nsMsgSearchScopeTerm *pScopeTerm = new nsMsgSearchScopeTerm(this, scope, folder);
  if (!pScopeTerm)
    return NS_ERROR_OUT_OF_MEMORY;

  m_scopeList.AppendElement(pScopeTerm);
  return NS_OK;
}

NS_IMETHODIMP
nsMsgSearchSession::AddDirectoryScopeTerm(nsMsgSearchScopeValue scope)
{
  nsMsgSearchScopeTerm *pScopeTerm = new nsMsgSearchScopeTerm(this, scope, nsnull);
  if (!pScopeTerm)
    return NS_ERROR_OUT_OF_MEMORY;

  m_scopeList.AppendElement(pScopeTerm);
  return NS_OK;
}

NS_IMETHODIMP nsMsgSearchSession::ClearScopes()
{
    DestroyScopeList();
    return NS_OK;
}

/* [noscript] boolean ScopeUsesCustomHeaders (in nsMsgSearchScope scope, in voidStar selection, in boolean forFilters); */
NS_IMETHODIMP
nsMsgSearchSession::ScopeUsesCustomHeaders(nsMsgSearchScopeValue scope,
                                           void * selection,
                                           bool forFilters,
                                           bool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean IsStringAttribute (in nsMsgSearchAttribute attrib); */
NS_IMETHODIMP
nsMsgSearchSession::IsStringAttribute(nsMsgSearchAttribValue attrib,
                                      bool *_retval)
{
  NS_ENSURE_ARG(_retval);
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void AddAllScopes (in nsMsgSearchScope attrib); */
NS_IMETHODIMP
nsMsgSearchSession::AddAllScopes(nsMsgSearchScopeValue attrib)
{
  // don't think this is needed.
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void Search (); */
NS_IMETHODIMP nsMsgSearchSession::Search(nsIMsgWindow *aWindow)
{
  nsresult rv = Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMsgSearchNotify> listener;
  m_iListener = 0;
  while (m_iListener != -1 && m_iListener < (signed)m_listenerList.Length())
  {
    listener = m_listenerList[m_iListener];
    PRInt32 listenerFlags = m_listenerFlagList[m_iListener++];
    if (!listenerFlags || (listenerFlags & nsIMsgSearchSession::onNewSearch))
      listener->OnNewSearch();
  }
  m_iListener = -1;

  m_msgWindowWeak = do_GetWeakReference(aWindow);

  return BeginSearching();
}

/* void InterruptSearch (); */
NS_IMETHODIMP nsMsgSearchSession::InterruptSearch()
{
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(m_msgWindowWeak));
  if (msgWindow)
  {
    EnableFolderNotifications(true);
    if (m_idxRunningScope < m_scopeList.Length())
      msgWindow->StopUrls();

    while (m_idxRunningScope < m_scopeList.Length())
    {
      ReleaseFolderDBRef();
      m_idxRunningScope++;
    }
    //m_idxRunningScope = m_scopeList.Length() so it will make us not run another url
  }
  if (m_backgroundTimer)
  {
    m_backgroundTimer->Cancel();
    NotifyListenersDone(NS_MSG_SEARCH_INTERRUPTED);

    m_backgroundTimer = nsnull;
  }
  return NS_OK;
}

NS_IMETHODIMP nsMsgSearchSession::PauseSearch()
{
  if (m_backgroundTimer)
  {
    m_backgroundTimer->Cancel();
    m_searchPaused = true;
    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsMsgSearchSession::ResumeSearch()
{
  if (m_searchPaused)
  {
    m_searchPaused = false;
    return StartTimer();
  }
  else
    return NS_ERROR_FAILURE;
}

/* [noscript] readonly attribute voidStar searchParam; */
NS_IMETHODIMP nsMsgSearchSession::GetSearchParam(void * *aSearchParam)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute nsMsgSearchType searchType; */
NS_IMETHODIMP nsMsgSearchSession::GetSearchType(nsMsgSearchType * *aSearchType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* [noscript] nsMsgSearchType SetSearchParam (in nsMsgSearchType type, in voidStar param); */
NS_IMETHODIMP nsMsgSearchSession::SetSearchParam(nsMsgSearchType *type, void * param, nsMsgSearchType **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute long numResults; */
NS_IMETHODIMP nsMsgSearchSession::GetNumResults(PRInt32 *aNumResults)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP nsMsgSearchSession::SetWindow(nsIMsgWindow *aWindow)
{
  m_msgWindowWeak = do_GetWeakReference(aWindow);
  return NS_OK;
}

NS_IMETHODIMP nsMsgSearchSession::GetWindow(nsIMsgWindow **aWindow)
{
  NS_ENSURE_ARG(aWindow);
  *aWindow = nsnull;
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(m_msgWindowWeak));
  msgWindow.swap(*aWindow);
  return NS_OK;
}

/* void OnStartRunningUrl (in nsIURI url); */
NS_IMETHODIMP nsMsgSearchSession::OnStartRunningUrl(nsIURI *url)
{
    return NS_OK;
}

/* void OnStopRunningUrl (in nsIURI url, in nsresult aExitCode); */
NS_IMETHODIMP nsMsgSearchSession::OnStopRunningUrl(nsIURI *url, nsresult aExitCode)
{
  nsCOMPtr <nsIMsgSearchAdapter> runningAdapter;

  nsresult rv = GetRunningAdapter (getter_AddRefs(runningAdapter));
  // tell the current adapter that the current url has run.
  if (NS_SUCCEEDED(rv) && runningAdapter)
  {
    runningAdapter->CurrentUrlDone(aExitCode);
    EnableFolderNotifications(true);
    ReleaseFolderDBRef();
  }
  m_idxRunningScope++;
  if (++m_urlQueueIndex < m_urlQueue.Length())
    GetNextUrl();
  else if (m_idxRunningScope < m_scopeList.Length())
    DoNextSearch();
  else
    NotifyListenersDone(aExitCode);
  return NS_OK;
}


nsresult nsMsgSearchSession::Initialize()
{
  // Loop over scope terms, initializing an adapter per term. This
  // architecture is necessitated by two things:
  // 1. There might be more than one kind of adapter per if online
  //    *and* offline mail mail folders are selected, or if newsgroups
  //    belonging to Dredd *and* INN are selected
  // 2. Most of the protocols are only capable of searching one scope at a
  //    time, so we'll do each scope in a separate adapter on the client

  nsMsgSearchScopeTerm *scopeTerm = nsnull;
  nsresult err = NS_OK;

  PRUint32 numTerms;
  m_termList->Count(&numTerms);
  // Ensure that the FE has added scopes and terms to this search
  NS_ASSERTION(numTerms > 0, "no terms to search!");
  if (numTerms == 0)
    return NS_MSG_ERROR_NO_SEARCH_VALUES;

  // if we don't have any search scopes to search, return that code.
  if (m_scopeList.Length() == 0)
    return NS_MSG_ERROR_INVALID_SEARCH_SCOPE;

  m_urlQueue.Clear(); // clear out old urls, if any.
  m_idxRunningScope = 0;
  m_urlQueueIndex = 0;

  // If this term list (loosely specified here by the first term) should be
  // scheduled in parallel, build up a list of scopes to do the round-robin scheduling
  for (PRUint32 i = 0; i < m_scopeList.Length() && NS_SUCCEEDED(err); i++)
  {
    scopeTerm = m_scopeList.ElementAt(i);
    // NS_ASSERTION(scopeTerm->IsValid());

    err = scopeTerm->InitializeAdapter (m_termList);
  }

  return err;
}

nsresult nsMsgSearchSession::BeginSearching()
{
  // Here's a sloppy way to start the URL, but I don't really have time to
  // unify the scheduling mechanisms. If the first scope is a newsgroup, and
  // it's not Dredd-capable, we build the URL queue. All other searches can be
  // done with one URL
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(m_msgWindowWeak));
  if (msgWindow)
    msgWindow->SetStopped(false);
  return DoNextSearch();
}

nsresult nsMsgSearchSession::DoNextSearch()
{
  nsMsgSearchScopeTerm *scope = m_scopeList.ElementAt(m_idxRunningScope);
  if (scope->m_attribute == nsMsgSearchScope::onlineMail ||
    (scope->m_attribute == nsMsgSearchScope::news && scope->m_searchServer))
    return BuildUrlQueue ();
  else
    return SearchWOUrls();
}


nsresult nsMsgSearchSession::BuildUrlQueue ()
{
  PRUint32 i;
  for (i = m_idxRunningScope; i < m_scopeList.Length(); i++)
  {
    nsMsgSearchScopeTerm *scope = m_scopeList.ElementAt(i);
    if (scope->m_attribute != nsMsgSearchScope::onlineMail &&
      (scope->m_attribute != nsMsgSearchScope::news && scope->m_searchServer))
      break;
    nsCOMPtr <nsIMsgSearchAdapter> adapter = do_QueryInterface(scope->m_adapter);
    if (adapter)
    {
      nsCString url;
      adapter->GetEncoding(getter_Copies(url));
      m_urlQueue.AppendElement(url);
    }
  }

  if (i > 0)
    GetNextUrl();

  return NS_OK;
}


nsresult nsMsgSearchSession::GetNextUrl()
{
  nsCString nextUrl;
  nsCOMPtr <nsIMsgMessageService> msgService;

  bool stopped = false;
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(m_msgWindowWeak));
  if (msgWindow)
    msgWindow->GetStopped(&stopped);
  if (stopped)
    return NS_OK;

  nextUrl = m_urlQueue[m_urlQueueIndex];
  nsMsgSearchScopeTerm *currentTerm = GetRunningScope();
  NS_ENSURE_TRUE(currentTerm, NS_ERROR_NULL_POINTER);
  EnableFolderNotifications(false);
  nsCOMPtr <nsIMsgFolder> folder = currentTerm->m_folder;
  if (folder)
  {
    nsCString folderUri;
    folder->GetURI(folderUri);
    nsresult rv = GetMessageServiceFromURI(folderUri, getter_AddRefs(msgService));

    if (NS_SUCCEEDED(rv) && msgService && currentTerm)
      msgService->Search(this, msgWindow, currentTerm->m_folder, nextUrl.get());

    return rv;
  }
  return NS_OK;
}

/* static */ void nsMsgSearchSession::TimerCallback(nsITimer *aTimer, void *aClosure)
{
  nsMsgSearchSession *searchSession = (nsMsgSearchSession *) aClosure;
  bool done;
  bool stopped = false;

  searchSession->TimeSlice(&done);
  nsCOMPtr<nsIMsgWindow> msgWindow(do_QueryReferent(searchSession->m_msgWindowWeak));
  if (msgWindow)
    msgWindow->GetStopped(&stopped);

  if (done || stopped)
  {
    aTimer->Cancel();
    searchSession->m_backgroundTimer = nsnull;
    if (searchSession->m_idxRunningScope < searchSession->m_scopeList.Length())
      searchSession->DoNextSearch();
    else
      searchSession->NotifyListenersDone(NS_OK);
  }
}

nsresult nsMsgSearchSession::StartTimer()
{
  nsresult rv;

  m_backgroundTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  m_backgroundTimer->InitWithFuncCallback(TimerCallback, (void *) this, 0,
                                          nsITimer::TYPE_REPEATING_SLACK);
  TimerCallback(m_backgroundTimer, this);
  return NS_OK;
}

nsresult nsMsgSearchSession::SearchWOUrls ()
{
  EnableFolderNotifications(false);
  return StartTimer();
}

NS_IMETHODIMP nsMsgSearchSession::GetRunningAdapter (nsIMsgSearchAdapter **aSearchAdapter)
{
  NS_ENSURE_ARG(aSearchAdapter);
  nsMsgSearchScopeTerm *scope = GetRunningScope();
  if (scope)
  {
    NS_ADDREF(*aSearchAdapter = scope->m_adapter);
    return NS_OK;
  }
  *aSearchAdapter = nsnull;
  return NS_OK;
}

NS_IMETHODIMP nsMsgSearchSession::AddSearchHit(nsIMsgDBHdr *aHeader,
                                               nsIMsgFolder *aFolder)
{
  nsCOMPtr<nsIMsgSearchNotify> listener;
  m_iListener = 0;
  while (m_iListener != -1 && m_iListener < (signed)m_listenerList.Length())
  {
    listener = m_listenerList[m_iListener];
    PRInt32 listenerFlags = m_listenerFlagList[m_iListener++];
    if (!listenerFlags || (listenerFlags & nsIMsgSearchSession::onSearchHit))
      listener->OnSearchHit(aHeader, aFolder);
  }
  m_iListener = -1;
  return NS_OK;
}

nsresult nsMsgSearchSession::NotifyListenersDone(nsresult aStatus)
{
  // need to stabilize "this" in case one of the listeners releases the last
  // reference to us.
  nsRefPtr <nsIMsgSearchSession> kungFuDeathGrip(this);

  nsCOMPtr<nsIMsgSearchNotify> listener;
  m_iListener = 0;
  while (m_iListener != -1 && m_iListener < (signed)m_listenerList.Length())
  {
    listener = m_listenerList[m_iListener];
    PRInt32 listenerFlags = m_listenerFlagList[m_iListener++];
    if (!listenerFlags || (listenerFlags & nsIMsgSearchSession::onSearchDone))
      listener->OnSearchDone(aStatus);
  }
  m_iListener = -1;
  return NS_OK;
}

void nsMsgSearchSession::DestroyScopeList()
{
  nsMsgSearchScopeTerm *scope = nsnull;

  for (PRInt32 i = m_scopeList.Length() - 1; i >= 0; i--)
  {
    scope = m_scopeList.ElementAt(i);
    //    NS_ASSERTION (scope->IsValid(), "invalid search scope");
    if (scope->m_adapter)
      scope->m_adapter->ClearScope();
    delete scope;
  }
  m_scopeList.Clear();
}


void nsMsgSearchSession::DestroyTermList ()
{
    m_termList->Clear();
}

nsMsgSearchScopeTerm *nsMsgSearchSession::GetRunningScope()
{
  return m_scopeList.SafeElementAt(m_idxRunningScope, nsnull);
}

nsresult nsMsgSearchSession::TimeSlice (bool *aDone)
{
  // we only do serial for now.
  return TimeSliceSerial(aDone);
}

void nsMsgSearchSession::ReleaseFolderDBRef()
{
  nsMsgSearchScopeTerm *scope = GetRunningScope();
  if (scope)
  {
    bool isOpen =false;
    PRUint32 flags;
    nsCOMPtr <nsIMsgFolder> folder;
    scope->GetFolder(getter_AddRefs(folder));
    nsCOMPtr <nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID);
    if (mailSession && folder)
    {
      mailSession->IsFolderOpenInWindow(folder, &isOpen);
      folder->GetFlags(&flags);

      /*we don't null out the db reference for inbox because inbox is like the "main" folder
       and performance outweighs footprint */
      if (!isOpen && !(nsMsgFolderFlags::Inbox & flags))
        folder->SetMsgDatabase(nsnull);
    }
  }
}
nsresult nsMsgSearchSession::TimeSliceSerial (bool *aDone)
{
  // This version of TimeSlice runs each scope term one at a time, and waits until one
  // scope term is finished before starting another one. When we're searching the local
  // disk, this is the fastest way to do it.

  NS_ENSURE_ARG(aDone);
  nsresult rv = NS_OK;
  nsMsgSearchScopeTerm *scope = GetRunningScope();
  if (scope)
  {
    rv = scope->TimeSlice (aDone);
    if (NS_FAILED(rv))
      *aDone = true;
    if (*aDone || NS_FAILED(rv))
    {
      EnableFolderNotifications(true);
      ReleaseFolderDBRef();
      m_idxRunningScope++;
      EnableFolderNotifications(false);
      // check if the next scope is an online search; if so,
      // set *aDone to true so that we'll try to run the next
      // search in TimerCallback.
      scope = GetRunningScope();
      if (scope && (scope->m_attribute == nsMsgSearchScope::onlineMail ||
        (scope->m_attribute == nsMsgSearchScope::news && scope->m_searchServer)))
      {
        *aDone = true;
        return rv;
      }

    }
    *aDone = false;
    return rv;
  }
  else
  {
    *aDone = true;
    return NS_OK;
  }
}

void
nsMsgSearchSession::EnableFolderNotifications(bool aEnable)
{
  nsMsgSearchScopeTerm *scope = GetRunningScope();
  if (scope)
  {
    nsCOMPtr<nsIMsgFolder> folder;
    scope->GetFolder(getter_AddRefs(folder));
    if (folder)  //enable msg count notifications
      folder->EnableNotifications(nsIMsgFolder::allMessageCountNotifications, aEnable, false);
  }
}

//this method is used for adding new hdrs to quick search view
NS_IMETHODIMP
nsMsgSearchSession::MatchHdr(nsIMsgDBHdr *aMsgHdr, nsIMsgDatabase *aDatabase, bool *aResult)
{
  nsMsgSearchScopeTerm *scope = m_scopeList.SafeElementAt(0, nsnull);
  if (scope)
  {
    if (!scope->m_adapter)
      scope->InitializeAdapter(m_termList);
    if (scope->m_adapter)
    {
      nsAutoString nullCharset, folderCharset;
      scope->m_adapter->GetSearchCharsets(nullCharset, folderCharset);
      NS_ConvertUTF16toUTF8 charset(folderCharset.get());
      nsMsgSearchOfflineMail::MatchTermsForSearch(aMsgHdr, m_termList, charset.get(), scope, aDatabase, &m_expressionTree, aResult);
    }
  }
  return NS_OK;
}

