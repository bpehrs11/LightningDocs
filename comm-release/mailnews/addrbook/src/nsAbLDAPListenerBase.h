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
 * Sun Microsystems, Inc.
 * Portions created by the Initial Developer are Copyright (C) 2001
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Paul Sandoz <paul.sandoz@sun.com>
 *   Dan Mosedale <dmose@mozilla.org>
 *   Mark Banner <mark@standard8.demon.co.uk>
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

#ifndef nsAbLDAPListenerBase_h__
#define nsAbLDAPListenerBase_h__

#include "nsCOMPtr.h"
#include "nsILDAPMessageListener.h"
#include "nsILDAPURL.h"
#include "nsILDAPConnection.h"
#include "nsILDAPOperation.h"
#include "nsStringGlue.h"
#include "mozilla/Mutex.h"

class nsAbLDAPListenerBase : public nsILDAPMessageListener
{
public:
  // Note that the directoryUrl is the details of the ldap directory
  // without any search params or attributes specified.
  nsAbLDAPListenerBase(nsILDAPURL* directoryUrl = nsnull,
                       nsILDAPConnection* connection = nsnull,
                       const nsACString &login = EmptyCString(),
                       const PRInt32 timeOut = 0);
  virtual ~nsAbLDAPListenerBase();

  NS_IMETHOD OnLDAPInit(nsILDAPConnection *aConn, nsresult aStatus);

protected:
  nsresult OnLDAPMessageBind(nsILDAPMessage *aMessage);

  nsresult Initiate();

  // Called if an LDAP initialization fails.
  virtual void InitFailed(bool aCancelled = false) = 0;

  // Called to start off the required task after a bind.
  virtual nsresult DoTask() = 0;

  nsCOMPtr<nsILDAPURL> mDirectoryUrl;
  nsCOMPtr<nsILDAPOperation> mOperation;        // current ldap op
  nsILDAPConnection* mConnection;
  nsCString mLogin;
  nsCString mSaslMechanism;
  PRInt32 mTimeOut;
  bool mBound;
  bool mInitialized;

  mozilla::Mutex mLock;
};

#endif
