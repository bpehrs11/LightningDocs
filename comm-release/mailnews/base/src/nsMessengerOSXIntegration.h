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

#ifndef __nsMessengerOSXIntegration_h
#define __nsMessengerOSXIntegration_h

#include "nsIMessengerOSIntegration.h"
#include "nsIFolderListener.h"
#include "nsIAtom.h"
#include "nsITimer.h"
#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsIObserver.h"
#include "nsIAlertsService.h"

#define NS_MESSENGEROSXINTEGRATION_CID \
  {0xaa83266, 0x4225, 0x4c4b, \
  {0x93, 0xf8, 0x94, 0xb1, 0x82, 0x58, 0x6f, 0x93}}

class nsIStringBundle;

class nsMessengerOSXIntegration : public nsIMessengerOSIntegration,
                                  public nsIFolderListener,
                                  public nsIObserver
{
public:
  nsMessengerOSXIntegration();
  virtual ~nsMessengerOSXIntegration();
  virtual nsresult Init();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIMESSENGEROSINTEGRATION
  NS_DECL_NSIFOLDERLISTENER
  NS_DECL_NSIOBSERVER

private:
  nsCOMPtr<nsIAtom> mBiffStateAtom;
  nsCOMPtr<nsIAtom> mNewMailReceivedAtom;
  nsCOMPtr<nsIAtom> mTotalUnreadMessagesAtom;
  nsresult ShowAlertMessage(const nsAString& aAlertTitle, const nsAString& aAlertText, const nsACString& aFolderURI);
  nsresult OnAlertFinished();
  nsresult OnAlertClicked(const PRUnichar * aAlertCookie);
  nsresult GetStringBundle(nsIStringBundle **aBundle);
  void FillToolTipInfo(nsIMsgFolder *aFolder, PRInt32 aNewCount);
  nsresult GetFirstFolderWithNewMail(nsIMsgFolder* aFolder, nsCString& aFolderURI);
  nsresult BadgeDockIcon();
  nsresult RestoreDockIcon();
  nsresult BounceDockIcon();
  nsresult GetNewMailAuthors(nsIMsgFolder* aFolder, nsString& aAuthors, PRInt32 aNewCount, PRInt32* aNotDisplayed);
  nsresult GetTotalUnread(nsIMsgFolder* aFolder, bool deep, PRInt32* aTotal);
  nsresult ConfirmShouldCount(nsIMsgFolder* aFolder, bool* aCountFolder);
  void InitUnreadCount();

  PRInt32 mUnreadTotal;
  PRInt32 mNewTotal;
  bool mOnlyCountInboxes;
  bool mDoneInitialCount;
};

#endif // __nsMessengerOSXIntegration_h
