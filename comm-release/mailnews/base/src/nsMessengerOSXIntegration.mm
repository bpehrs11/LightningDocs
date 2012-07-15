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
 * The Original Code is Mac OSX New Mail Notification Code..
 *
 * The Initial Developer of the Original Code is
 * The Mozilla Foundation.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Scott MacGregor <mscott@mozilla.org>
 *  Jon Baumgartner <jon@bergenstreetsoftware.com>
 *  David Humphrey <david.humphrey@senecac.on.ca>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
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

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG /* Allow logging in the release build */
#endif

#include "nscore.h"
#include "nsMsgUtils.h"
#include "nsMessengerOSXIntegration.h"
#include "nsIMsgMailSession.h"
#include "nsIMsgIncomingServer.h"
#include "nsIMsgIdentity.h"
#include "nsIMsgAccount.h"
#include "nsIMsgFolder.h"
#include "nsCOMPtr.h"
#include "nsMsgBaseCID.h"
#include "nsMsgFolderFlags.h"
#include "nsDirectoryServiceDefs.h"
#include "nsIDirectoryService.h"
#include "MailNewsTypes.h"
#include "nsIWindowMediator.h"
#include "nsIDOMChromeWindow.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDocShell.h"
#include "nsIBaseWindow.h"
#include "nsIWidget.h"
#include "nsIObserverService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIMessengerWindowService.h"
#include "prprf.h"
#include "prlog.h"
#include "nsIAlertsService.h"
#include "nsIStringBundle.h"
#include "nsToolkitCompsCID.h"
#include "nsINotificationsList.h"
#include "nsIMsgDatabase.h"
#include "nsIMsgHdr.h"
#include "nsIMsgHeaderParser.h"
#include "nsISupportsPrimitives.h"
#include "nsIWindowWatcher.h"
#include "nsMsgLocalCID.h"
#include "nsIMsgMailNewsUrl.h"
#include "nsIMsgWindow.h"
#include "nsIMsgAccountManager.h"
#include "nsIMessenger.h"
#include "nsObjCExceptions.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"

#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

#define kNewMailAlertIcon "chrome://messenger/skin/icons/new-mail-alert.png"
#define kBiffShowAlertPref "mail.biff.show_alert"
#define kCountInboxesPref "mail.notification.count.inbox_only"
#define kMaxDisplayCount 10

static PRLogModuleInfo *MsgDockCountsLogModule = nsnull;

// HACK: Limitations in Focus/SetFocus on Mac (see bug 465446)
nsresult FocusAppNative()
{
  ProcessSerialNumber psn;

  if (::GetCurrentProcess(&psn) != 0)
   return NS_ERROR_FAILURE;

  if (::SetFrontProcess(&psn) != 0)
   return NS_ERROR_FAILURE;

  return NS_OK;
}

static void openMailWindow(const nsCString& aUri)
{
  nsresult rv;
  nsCOMPtr<nsIMsgMailSession> mailSession ( do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv));
  if (NS_FAILED(rv))
    return;

  nsCOMPtr<nsIMsgWindow> topMostMsgWindow;
  rv = mailSession->GetTopmostMsgWindow(getter_AddRefs(topMostMsgWindow));
  if (topMostMsgWindow)
  {
    if (!aUri.IsEmpty())
    {
      nsCOMPtr<nsIMsgMailNewsUrl> msgUri(do_CreateInstance(NS_MAILBOXURL_CONTRACTID, &rv));
      if (NS_FAILED(rv))
        return;

      rv = msgUri->SetSpec(aUri);
      if (NS_FAILED(rv))
        return;

      bool isMessageUri = false;
      msgUri->GetIsMessageUri(&isMessageUri);
      if (isMessageUri)
      {
        nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID, &rv));
        if (NS_FAILED(rv))
          return;

        // SeaMonkey only supports message uris, whereas Thunderbird only
        // supports message headers. This should be simplified/removed when
        // bug 507593 is implemented.
#ifdef MOZ_SUITE
        nsCOMPtr<nsIDOMWindow> newWindow;
        wwatch->OpenWindow(0, "chrome://messenger/content/messageWindow.xul",
                           "_blank", "all,chrome,dialog=no,status,toolbar", msgUri,
                           getter_AddRefs(newWindow));
#else
        nsCOMPtr<nsIMessenger> messenger(do_CreateInstance(NS_MESSENGER_CONTRACTID, &rv));
        if (NS_FAILED(rv))
          return;

        nsCOMPtr<nsIMsgDBHdr> msgHdr; 
        messenger->MsgHdrFromURI(aUri, getter_AddRefs(msgHdr));
        if (msgHdr)
        {
          nsCOMPtr<nsIDOMWindow> newWindow;
          wwatch->OpenWindow(0, "chrome://messenger/content/messageWindow.xul",
                             "_blank", "all,chrome,dialog=no,status,toolbar", msgHdr,
                             getter_AddRefs(newWindow));
        }
#endif
      }
      else
      {
        nsCOMPtr<nsIMsgWindowCommands> windowCommands;
        topMostMsgWindow->GetWindowCommands(getter_AddRefs(windowCommands));
        if (windowCommands)
          windowCommands->SelectFolder(aUri);
      }
    }

    FocusAppNative();
    nsCOMPtr<nsIDOMWindow> domWindow;
    topMostMsgWindow->GetDomWindow(getter_AddRefs(domWindow));
    if (domWindow)
      domWindow->Focus();
  }
  else
  {
    // the user doesn't have a mail window open already so open one for them...
    nsCOMPtr<nsIMessengerWindowService> messengerWindowService =
      do_GetService(NS_MESSENGERWINDOWSERVICE_CONTRACTID);
    // if we want to preselect the first account with new mail,
    // here is where we would try to generate a uri to pass in
    // (and add code to the messenger window service to make that work)
    if (messengerWindowService)
      messengerWindowService->OpenMessengerWindowWithUri(
                                "mail:3pane", aUri.get(), nsMsgKey_None);
  }
}

nsMessengerOSXIntegration::nsMessengerOSXIntegration()
{
  mBiffStateAtom = MsgGetAtom("BiffState");
  mNewMailReceivedAtom = MsgGetAtom("NewMailReceived");
  mTotalUnreadMessagesAtom = MsgGetAtom("TotalUnreadMessages");
  mUnreadTotal = 0;
  mNewTotal = 0;
  mOnlyCountInboxes = true;
  mDoneInitialCount = false;
}

nsMessengerOSXIntegration::~nsMessengerOSXIntegration()
{
  RestoreDockIcon();
}

NS_IMPL_ADDREF(nsMessengerOSXIntegration)
NS_IMPL_RELEASE(nsMessengerOSXIntegration)

NS_INTERFACE_MAP_BEGIN(nsMessengerOSXIntegration)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIMessengerOSIntegration)
   NS_INTERFACE_MAP_ENTRY(nsIMessengerOSIntegration)
   NS_INTERFACE_MAP_ENTRY(nsIFolderListener)
   NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END


nsresult
nsMessengerOSXIntegration::Init()
{
  // need to register a named Growl notification
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1", &rv);
  if (NS_SUCCEEDED(rv))
  {
    observerService->AddObserver(this, "before-growl-registration", false);
    observerService->AddObserver(this, "mail-startup-done", false);
  }

  nsCOMPtr<nsIMsgMailSession> mailSession = do_GetService(NS_MSGMAILSESSION_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // because we care if the unread total count changes
  return mailSession->AddFolderListener(this, nsIFolderListener::boolPropertyChanged | nsIFolderListener::intPropertyChanged);
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemPropertyChanged(nsIMsgFolder *, nsIAtom *, char const *, char const *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemUnicharPropertyChanged(nsIMsgFolder *, nsIAtom *, const PRUnichar *, const PRUnichar *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemRemoved(nsIMsgFolder *, nsISupports *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::Observe(nsISupports* aSubject, const char* aTopic, const PRUnichar* aData)
{
  if (!strcmp(aTopic, "alertfinished"))
    return OnAlertFinished();

  if (!strcmp(aTopic, "alertclickcallback"))
    return OnAlertClicked(aData);

  // get the initial unread count for the dock icon and badge
  if (!strcmp(aTopic, "mail-startup-done"))
  {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_SUCCEEDED(rv))
      observerService->RemoveObserver(this, "mail-startup-done");
    InitUnreadCount();
    BadgeDockIcon();
  }

  // register named Growl notification for new mail alerts.
  if (!strcmp(aTopic, "before-growl-registration"))
  {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService = do_GetService("@mozilla.org/observer-service;1", &rv);
    if (NS_SUCCEEDED(rv))
      observerService->RemoveObserver(this, "before-growl-registration");

    nsCOMPtr<nsINotificationsList> notifications = do_QueryInterface(aSubject, &rv);
    if (NS_SUCCEEDED(rv))
    {
      nsCOMPtr<nsIStringBundle> bundle;
      GetStringBundle(getter_AddRefs(bundle));
      if (bundle)
      {
        nsString growlNotification;
        bundle->GetStringFromName(NS_LITERAL_STRING("growlNotification").get(), getter_Copies(growlNotification));
        notifications->AddNotification(growlNotification, true);
      }
    }
  }
  return NS_OK;
}

nsresult
nsMessengerOSXIntegration::GetStringBundle(nsIStringBundle **aBundle)
{
  NS_ENSURE_ARG_POINTER(aBundle);
  nsresult rv;
  nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  nsCOMPtr<nsIStringBundle> bundle;
  if (bundleService && NS_SUCCEEDED(rv))
    bundleService->CreateBundle("chrome://messenger/locale/messenger.properties", getter_AddRefs(bundle));
  bundle.swap(*aBundle);
  return rv;
}

void
nsMessengerOSXIntegration::FillToolTipInfo(nsIMsgFolder *aFolder, PRInt32 aNewCount)
{
  if (aFolder)
  {
    nsString authors;
    PRInt32 numNotDisplayed;
    nsresult rv = GetNewMailAuthors(aFolder, authors, aNewCount, &numNotDisplayed);

    // If all senders are vetoed, the authors string will be empty.
    if (NS_FAILED(rv) || authors.IsEmpty())
      return;

    // If this isn't the root folder, get it so we can report for it.
    // GetRootFolder always returns the server's root, so calling on the root itself is fine.
    nsCOMPtr<nsIMsgFolder> rootFolder;
    aFolder->GetRootFolder(getter_AddRefs(rootFolder));
    if (!rootFolder)
      return;

    nsString accountName;
    rootFolder->GetPrettiestName(accountName);

    nsCOMPtr<nsIStringBundle> bundle;
    GetStringBundle(getter_AddRefs(bundle));
    if (bundle)
    {
      nsAutoString numNewMsgsText;
      numNewMsgsText.AppendInt(aNewCount);
      nsString finalText;
      nsCString uri;
      aFolder->GetURI(uri);

      if (numNotDisplayed > 0)
      {
        nsAutoString numNotDisplayedText;
        numNotDisplayedText.AppendInt(numNotDisplayed);
        const PRUnichar *formatStrings[3] = { numNewMsgsText.get(), authors.get(), numNotDisplayedText.get() };
        bundle->FormatStringFromName(NS_LITERAL_STRING("macBiffNotification_messages_extra").get(),
                                     formatStrings,
                                     3,
                                     getter_Copies(finalText));
      }
      else
      {
        const PRUnichar *formatStrings[2] = { numNewMsgsText.get(), authors.get() };

        if (aNewCount == 1)
        {
          bundle->FormatStringFromName(NS_LITERAL_STRING("macBiffNotification_message").get(),
                                       formatStrings,
                                       2,
                                       getter_Copies(finalText));
          // Since there is only 1 message, use the most recent mail's URI instead of the folder's
          nsCOMPtr<nsIMsgDatabase> db;
          rv = aFolder->GetMsgDatabase(getter_AddRefs(db));
          if (NS_SUCCEEDED(rv) && db)
          {
            PRUint32 numNewKeys;
            PRUint32 *newMessageKeys;
            rv = db->GetNewList(&numNewKeys, &newMessageKeys);
            if (NS_SUCCEEDED(rv))
            {
              nsCOMPtr<nsIMsgDBHdr> hdr;
              rv = db->GetMsgHdrForKey(newMessageKeys[numNewKeys - 1],
                                       getter_AddRefs(hdr));
              if (NS_SUCCEEDED(rv) && hdr)
                aFolder->GetUriForMsg(hdr, uri);
            }
            NS_Free(newMessageKeys);
          }
        }
        else
          bundle->FormatStringFromName(NS_LITERAL_STRING("macBiffNotification_messages").get(),
                                       formatStrings,
                                       2,
                                       getter_Copies(finalText));
      }
      ShowAlertMessage(accountName, finalText, uri);
    } // if we got a bundle
  } // if we got a folder
}

nsresult
nsMessengerOSXIntegration::ShowAlertMessage(const nsAString& aAlertTitle,
                                            const nsAString& aAlertText,
                                            const nsACString& aFolderURI)
{
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  bool showAlert = true;
  prefBranch->GetBoolPref(kBiffShowAlertPref, &showAlert);

  if (showAlert)
  {
    // Use growl if installed
    nsCOMPtr<nsIAlertsService> alertsService (do_GetService(NS_ALERTSERVICE_CONTRACTID, &rv));
    if (NS_SUCCEEDED(rv))
    {
      nsCOMPtr<nsIStringBundle> bundle;
      GetStringBundle(getter_AddRefs(bundle));
      if (bundle)
      {
        nsString growlNotification;
        bundle->GetStringFromName(NS_LITERAL_STRING("growlNotification").get(),
                                  getter_Copies(growlNotification));
        rv = alertsService->ShowAlertNotification(NS_LITERAL_STRING(kNewMailAlertIcon),
                                                  aAlertTitle,
                                                  aAlertText,
                                                  true,
                                                  NS_ConvertASCIItoUTF16(aFolderURI),
                                                  this,
                                                  growlNotification);
      }
    }

    bool bounceDockIcon = false;
    prefBranch->GetBoolPref("mail.biff.animate_dock_icon", &bounceDockIcon);

    if (bounceDockIcon)
      BounceDockIcon();
  }

  if (!showAlert || NS_FAILED(rv))
    OnAlertFinished();

  return rv;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemIntPropertyChanged(nsIMsgFolder *aFolder,
                                                    nsIAtom *aProperty,
                                                    PRInt32 aOldValue,
                                                    PRInt32 aNewValue)
{
   // if we got new mail show an alert
  if (mBiffStateAtom == aProperty)
  {
    NS_ENSURE_TRUE(aFolder, NS_OK);

    if (aNewValue == nsIMsgFolder::nsMsgBiffState_NewMail)
    {
      bool performingBiff = false;
      nsCOMPtr<nsIMsgIncomingServer> server;
      aFolder->GetServer(getter_AddRefs(server));
      if (server)
        server->GetPerformingBiff(&performingBiff);
      if (!performingBiff)
        return NS_OK; // kick out right now...

      // Biff happens for the root folder, but we want info for the child with new mail
      nsCString folderUri;
      GetFirstFolderWithNewMail(aFolder, folderUri);
      nsCOMPtr<nsIMsgFolder> childFolder;
      nsresult rv = aFolder->GetChildWithURI(folderUri, true, true,
                                             getter_AddRefs(childFolder));
      if (NS_FAILED(rv) || !childFolder)
        return NS_ERROR_FAILURE;

      PRInt32 numNewMessages = 0;
      childFolder->GetNumNewMessages(true, &numNewMessages);
      FillToolTipInfo(childFolder, numNewMessages);

      mNewTotal += numNewMessages;
      BadgeDockIcon();
    }
    else if (aNewValue == nsIMsgFolder::nsMsgBiffState_NoMail)
    {
      // reset new message total
      mNewTotal = 0;
      BadgeDockIcon();
    }
  }
  else if (mNewMailReceivedAtom == aProperty)
  {
    nsCOMPtr<nsIMsgFolder> rootFolder;
    nsresult rv = aFolder->GetRootFolder(getter_AddRefs(rootFolder));
    NS_ENSURE_SUCCESS(rv, rv);

    FillToolTipInfo(aFolder, aNewValue);

    mNewTotal += aNewValue;
    BadgeDockIcon();
  }
  else if (mTotalUnreadMessagesAtom == aProperty)
  {
    // Insure we have already done an initial count so we don't double-count this change.
    if (!mDoneInitialCount)
    {
      InitUnreadCount();
      BadgeDockIcon();
      // Return early, since the change below has just been counted.
      return NS_OK;
    }

    PRUint32 flags;
    nsresult rv = aFolder->GetFlags(&flags);
    NS_ENSURE_SUCCESS(rv, rv);

    // Count this folder if: 1) we want only inboxes and this is an inbox; or
    // 2) we want any folder (see ConfirmShouldCount() for folders included).
    if ((mOnlyCountInboxes && flags & nsMsgFolderFlags::Inbox) || !mOnlyCountInboxes)
    {
      // Give extensions a chance to suppress counting for this folder,
      // and filter out ones we don't want to count.
      bool countFolder;
      rv = ConfirmShouldCount(aFolder, &countFolder);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!countFolder)
        return NS_OK;

      // Increment count by difference, treating -1 (i.e., "don't know") as 0
      mUnreadTotal += aNewValue - (aOldValue > -1 ? aOldValue : 0);
      nsCString folderURI;
      aFolder->GetURI(folderURI);
      PR_LOG(MsgDockCountsLogModule, PR_LOG_ALWAYS,
             ("changing unread to %d aNewValue = %d oldValue = %d for folder %s", 
               mUnreadTotal, aNewValue, aOldValue, folderURI.get()));
      
      NS_ASSERTION(mUnreadTotal > -1, "Updated unread message count is less than zero.");

      BadgeDockIcon();
    }
  }
  return NS_OK;
}

nsresult
nsMessengerOSXIntegration::OnAlertClicked(const PRUnichar* aAlertCookie)
{
  openMailWindow(NS_ConvertUTF16toUTF8(aAlertCookie));
  return NS_OK;
}

nsresult
nsMessengerOSXIntegration::OnAlertFinished()
{
  return NS_OK;
}

nsresult
nsMessengerOSXIntegration::BounceDockIcon()
{
  nsCOMPtr<nsIWindowMediator> mediator(do_GetService(NS_WINDOWMEDIATOR_CONTRACTID));
  if (mediator)
  {
    nsCOMPtr<nsIDOMWindow> domWindow;
    mediator->GetMostRecentWindow(NS_LITERAL_STRING("mail:3pane").get(), getter_AddRefs(domWindow));
    if (domWindow)
    {
      nsCOMPtr<nsIDOMChromeWindow> chromeWindow(do_QueryInterface(domWindow));
      chromeWindow->GetAttention();
    }
  }
  return NS_OK;
}

nsresult
nsMessengerOSXIntegration::RestoreDockIcon()
{
  id tile = [[NSApplication sharedApplication] dockTile];
  [tile setBadgeLabel: nil];

  return NS_OK;
}

nsresult
nsMessengerOSXIntegration::BadgeDockIcon()
{
  // Use either unread messages as the count, or new messages as the count, depending on the preference
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);
  bool useNewCount = false;
  prefBranch->GetBoolPref("mail.biff.use_new_count_in_mac_dock", &useNewCount);

  // If count is less than one, we should restore the original dock icon.
  if ((!useNewCount && mUnreadTotal < 1) || (useNewCount && mNewTotal < 1))
  {
    RestoreDockIcon();
    return NS_OK;
  }

  // Draw the number, first giving extensions a chance to modify.
  // Extensions might wish to transform "1000" into "100+" or some
  // other short string. Getting back the empty string will cause
  // nothing to be drawn and us to return early.
  nsCOMPtr<nsIObserverService> os
    (do_GetService("@mozilla.org/observer-service;1", &rv));
  if (NS_FAILED(rv))
  {
    RestoreDockIcon();
    return rv;
  }

  nsCOMPtr<nsISupportsString> str
    (do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv));
  if (NS_FAILED(rv))
  {
    RestoreDockIcon();
    return rv;
  }

  nsAutoString total;

  if (useNewCount)
    total.AppendInt(mNewTotal);
  else
    total.AppendInt(mUnreadTotal);

  str->SetData(total);
  os->NotifyObservers(str, "before-unread-count-display",
                      total.get());
  nsAutoString badgeString;
  str->GetData(badgeString);
  if (badgeString.IsEmpty())
  {
    RestoreDockIcon();
    return NS_OK;
  }

  id tile = [[NSApplication sharedApplication] dockTile];
  [tile setBadgeLabel:[NSString stringWithFormat:@"%S", total.get()]];
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemPropertyFlagChanged(nsIMsgDBHdr *item, nsIAtom *property, PRUint32 oldFlag, PRUint32 newFlag)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemAdded(nsIMsgFolder *, nsISupports *)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemBoolPropertyChanged(nsIMsgFolder *aItem,
                                                         nsIAtom *aProperty,
                                                         bool aOldValue,
                                                         bool aNewValue)
{
  return NS_OK;
}

NS_IMETHODIMP
nsMessengerOSXIntegration::OnItemEvent(nsIMsgFolder *, nsIAtom *)
{
  return NS_OK;
}

nsresult
nsMessengerOSXIntegration::GetNewMailAuthors(nsIMsgFolder* aFolder,
                                             nsString& aAuthors,
                                             PRInt32 aNewCount,
                                             PRInt32* aNotDisplayed)
{
  // Get a list of names or email addresses for the folder's authors
  // with new mail. Note that we only process the most recent "new"
  // mail (aNewCount), working from most recently added. Duplicates
  // are removed, and names are displayed to a set limit
  // (kMaxDisplayCount) with the remaining count being returned in
  // aNotDisplayed. Extension developers can listen for
  // "newmail-notification-requested" and then make a decision about
  // including a given author or not. As a result, it is possible that
  // the resulting length of aAuthors will be 0.
  nsCOMPtr<nsIMsgDatabase> db;
  nsresult rv = aFolder->GetMsgDatabase(getter_AddRefs(db));
  PRUint32 numNewKeys = 0;
  if (NS_SUCCEEDED(rv) && db)
  {
    nsCOMPtr<nsIMsgHeaderParser> parser =
      do_GetService(NS_MAILNEWS_MIME_HEADER_PARSER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserverService> os =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get proper l10n list separator -- ", " in English
    nsCOMPtr<nsIStringBundle> bundle;
    GetStringBundle(getter_AddRefs(bundle));
    if (!bundle)
      return NS_ERROR_FAILURE;

    PRUint32 *newMessageKeys;
    rv = db->GetNewList(&numNewKeys, &newMessageKeys);
    if (NS_SUCCEEDED(rv))
    {
      nsString listSeparator;
      bundle->GetStringFromName(NS_LITERAL_STRING("macBiffNotification_separator").get(), getter_Copies(listSeparator));

      PRInt32 displayed = 0;
      for (PRInt32 i = numNewKeys - 1; i >= 0; i--, aNewCount--)
      {
        if (0 == aNewCount || displayed == kMaxDisplayCount)
          break;

        nsCOMPtr<nsIMsgDBHdr> hdr;
        rv = db->GetMsgHdrForKey(newMessageKeys[i],
                                 getter_AddRefs(hdr));
        if (NS_SUCCEEDED(rv) && hdr)
        {
          nsString author;
          rv = hdr->GetMime2DecodedAuthor(author);
          if (NS_FAILED(rv))
            continue;

          nsCString name;
          rv = parser->ExtractHeaderAddressName(NS_ConvertUTF16toUTF8(author),
                                                name);
          if (NS_FAILED(rv))
            continue;

          // Give extensions a chance to suppress notifications for this author
          nsCOMPtr<nsISupportsPRBool> notify =
            do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID);

          notify->SetData(true);
          os->NotifyObservers(notify, "newmail-notification-requested",
                              PromiseFlatString(author).get());

          bool includeSender;
          notify->GetData(&includeSender);

          // Don't add unwanted or duplicate names
          if (includeSender &&
              aAuthors.Find(name.get(), true) == -1)
          {
            if (displayed > 0)
              aAuthors.Append(listSeparator);
            aAuthors.Append(NS_ConvertUTF8toUTF16(name));
            displayed++;
          }
        }
      }
    }
    NS_Free(newMessageKeys);
  }
  *aNotDisplayed = aNewCount;
  return rv;
}

nsresult
nsMessengerOSXIntegration::GetFirstFolderWithNewMail(nsIMsgFolder* aFolder, nsCString& aFolderURI)
{
  // Find the subfolder in aFolder with new mail and return the folderURI
  if (aFolder)
  {
    nsCOMPtr<nsIMsgFolder> msgFolder;
    // enumerate over the folders under this root folder till we find one with new mail....
    nsCOMPtr<nsISupportsArray> allFolders;
    NS_NewISupportsArray(getter_AddRefs(allFolders));
    nsresult rv = aFolder->ListDescendents(allFolders);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIEnumerator> enumerator;
    allFolders->Enumerate(getter_AddRefs(enumerator));
    if (enumerator)
    {
      nsCOMPtr<nsISupports> supports;
      PRInt32 numNewMessages = 0;
      nsresult more = enumerator->First();
      while (NS_SUCCEEDED(more))
      {
        rv = enumerator->CurrentItem(getter_AddRefs(supports));
        if (supports)
        {
          msgFolder = do_QueryInterface(supports, &rv);
          if (msgFolder)
          {
            numNewMessages = 0;
            msgFolder->GetNumNewMessages(false, &numNewMessages);
            if (numNewMessages)
              break; // kick out of the while loop
            more = enumerator->Next();
          }
        } // if we have a folder
      }  // if we have more potential folders to enumerate
    }  // if enumerator

    if (msgFolder)
      msgFolder->GetURI(aFolderURI);
  }

  return NS_OK;
}

void
nsMessengerOSXIntegration::InitUnreadCount()
{
  // If we were forced to do a count early with an update, don't do it again on mail startup.
  if (mDoneInitialCount)
    return;
  mDoneInitialCount = true;
  if (!MsgDockCountsLogModule)
    MsgDockCountsLogModule = PR_NewLogModule("DockCounts");
  // We either count just inboxes, or all folders
  nsresult rv;
  nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, );

  rv = prefBranch->GetBoolPref(kCountInboxesPref, &mOnlyCountInboxes);
  NS_ENSURE_SUCCESS(rv, );

  nsCOMPtr<nsIMsgAccountManager> accountManager =
    do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, );

  nsCOMPtr<nsISupportsArray> servers;
  rv = accountManager->GetAllServers(getter_AddRefs(servers));
  NS_ENSURE_SUCCESS(rv, );

  PRUint32 count;
  rv = servers->Count(&count);
  NS_ENSURE_SUCCESS(rv, );

  mUnreadTotal = 0;

  PRUint32 i;
  for (i = 0; i < count; i++)
  {
    nsCOMPtr<nsIMsgIncomingServer> server = do_QueryElementAt(servers, i);
    if (!server)
      continue;

    nsCOMPtr<nsIMsgFolder> rootFolder;
    server->GetRootFolder(getter_AddRefs(rootFolder));
    if (!rootFolder)
      continue;

    // Get a combined unread count for all desired folders
    PRInt32 numUnread = 0;
    if (mOnlyCountInboxes)
    {
      nsCOMPtr<nsIMsgFolder> inboxFolder;
      rootFolder->GetFolderWithFlags(nsMsgFolderFlags::Inbox, getter_AddRefs(inboxFolder));
      if (inboxFolder)
      {
        GetTotalUnread(inboxFolder, false, &numUnread);
        nsCString folderURI;
        inboxFolder->GetURI(folderURI);
        PR_LOG(MsgDockCountsLogModule, PR_LOG_ALWAYS,
               ("adding %d unread from %s", numUnread, folderURI.get()));
      }
    }
    else
    {
      GetTotalUnread(rootFolder, true, &numUnread);
      nsCString folderURI;
      rootFolder->GetURI(folderURI);
      PR_LOG(MsgDockCountsLogModule, PR_LOG_ALWAYS,
             ("adding %d unread from %s", numUnread, folderURI.get()));
      
    }

    mUnreadTotal += numUnread;
     NS_ASSERTION(mUnreadTotal > -1, "Initial unread message count is less than zero.");
  }
  PR_LOG(MsgDockCountsLogModule, PR_LOG_ALWAYS,
         ("initial unread count = %d", mUnreadTotal));
}

nsresult
nsMessengerOSXIntegration::ConfirmShouldCount(nsIMsgFolder* aFolder, bool* aCountFolder)
{
  // We give extensions a chance to say yes/no to counting for a folder.  By
  // default we count every folder that is mail and isn't 
  // Trash, Junk, Drafts, "Outbox", or a Virtual folder.
  nsCOMPtr<nsIMsgIncomingServer> server;
  nsresult rv = aFolder->GetServer(getter_AddRefs(server));
  NS_ENSURE_SUCCESS(rv, rv);

  bool defaultValue = true;
  nsCAutoString type;
  rv = server->GetType(type);
  if (NS_FAILED(rv) || (type.EqualsLiteral("rss") || type.EqualsLiteral("nntp")))
  {
    defaultValue = false;
    return NS_OK;
  }

  nsCOMPtr<nsIObserverService> os =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsPRBool> shouldCount =
    do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 flags;
  aFolder->GetFlags(&flags);
  if ((flags & nsMsgFolderFlags::Trash)   ||
      (flags & nsMsgFolderFlags::Drafts)  ||
      (flags & nsMsgFolderFlags::Queue)   ||
      (flags & nsMsgFolderFlags::Virtual) ||
      (flags & nsMsgFolderFlags::Junk))
    defaultValue = false;

  rv = shouldCount->SetData(defaultValue);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCString folderUri;
  rv = aFolder->GetURI(folderUri);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = os->NotifyObservers(shouldCount, "before-count-unread-for-folder",
                           NS_ConvertUTF8toUTF16(folderUri).get());
  NS_ENSURE_SUCCESS(rv, rv);

  return shouldCount->GetData(aCountFolder);
}

nsresult
nsMessengerOSXIntegration::GetTotalUnread(nsIMsgFolder* aFolder, bool deep, PRInt32* aTotal)
{
  // This simulates nsIMsgFolder::GetNumUnread, but gives extensions
  // a chance to decide whether folders should be counted as part of
  // the total.
  *aTotal = 0;
  bool countFolder;
  nsresult rv = ConfirmShouldCount(aFolder, &countFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!countFolder)
    return NS_OK;

  PRInt32 total = 0;
  rv = aFolder->GetNumUnread(false, &total);
  NS_ENSURE_SUCCESS(rv, rv);

  // Use zero instead of -1 (don't know) or other special nums.
  total = total >= 0 ? total : 0;

  if (deep)
  {
    bool hasChildren;
    rv = aFolder->GetHasSubFolders(&hasChildren);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 flags;
    aFolder->GetFlags(&flags);

    if (hasChildren && !(flags & nsMsgFolderFlags::Virtual))
    {
      nsCOMPtr<nsISimpleEnumerator> children;
      rv = aFolder->GetSubFolders(getter_AddRefs(children));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIMsgFolder> childFolder;
      bool moreFolders;
      while (NS_SUCCEEDED(children->HasMoreElements(&moreFolders)) &&
             moreFolders)
      {
        nsCOMPtr<nsISupports> child;
        rv = children->GetNext(getter_AddRefs(child));
        if (NS_SUCCEEDED(rv) && child)
        {
          childFolder = do_QueryInterface(child, &rv);
          if (NS_SUCCEEDED(rv) && childFolder)
          {
            PRInt32 childFolderCount = 0;
            rv = GetTotalUnread(childFolder, true, &childFolderCount);
            if (NS_FAILED(rv))
              continue;

            total += childFolderCount;
          }
        }
      }
    }
  }
  *aTotal = total;
  return NS_OK;
}
