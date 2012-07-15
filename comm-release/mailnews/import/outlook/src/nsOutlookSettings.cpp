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

/*

  Outlook Express (Win32) settings

*/

#include "nsCOMPtr.h"
#include "nscore.h"
#include "nsMsgUtils.h"
#include "nsOutlookImport.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIImportService.h"
#include "nsOutlookRegUtil.h"
#include "nsIMsgAccountManager.h"
#include "nsIMsgAccount.h"
#include "nsIImportSettings.h"
#include "nsOutlookSettings.h"
#include "nsMsgBaseCID.h"
#include "nsMsgCompCID.h"
#include "nsISmtpService.h"
#include "nsISmtpServer.h"
#include "nsOutlookStringBundle.h"
#include "OutlookDebugLog.h"
#include "nsIPop3IncomingServer.h"
#include "nsMsgI18N.h"

class OutlookSettings {
public:
  static HKEY FindAccountsKey( void);

  static bool DoImport( nsIMsgAccount **ppAccount);

  static bool DoIMAPServer( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount);
  static bool DoPOP3Server( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount);

  static void SetIdentities( nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, HKEY hKey);
  static bool IdentityMatches( nsIMsgIdentity *pIdent, const char *pName, const char *pServer, const char *pEmail, const char *pReply, const char *pUserName);

  static void SetSmtpServer( nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, char *pServer, const nsCString& pUser);
  static nsresult GetAccountName(HKEY hKey, char *defaultName, nsString &acctName);
};


////////////////////////////////////////////////////////////////////////
nsresult nsOutlookSettings::Create(nsIImportSettings** aImport)
{
    NS_PRECONDITION(aImport != nsnull, "null ptr");
    if (! aImport)
        return NS_ERROR_NULL_POINTER;

    *aImport = new nsOutlookSettings();
    if (! *aImport)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(*aImport);
    return NS_OK;
}

nsOutlookSettings::nsOutlookSettings()
{
}

nsOutlookSettings::~nsOutlookSettings()
{
}

NS_IMPL_ISUPPORTS1(nsOutlookSettings, nsIImportSettings)

NS_IMETHODIMP nsOutlookSettings::AutoLocate(PRUnichar **description, nsIFile **location, bool *_retval)
{
    NS_PRECONDITION(description != nsnull, "null ptr");
    NS_PRECONDITION(_retval != nsnull, "null ptr");
  if (!description || !_retval)
    return( NS_ERROR_NULL_POINTER);

  *description = nsOutlookStringBundle::GetStringByID( OUTLOOKIMPORT_NAME);
  *_retval = false;

  if (location)
    *location = nsnull;

  // look for the registry key for the accounts
  HKEY key = OutlookSettings::FindAccountsKey();
  if (key != nsnull) {
    *_retval = true;
    ::RegCloseKey( key);
  }

  return( NS_OK);
}

NS_IMETHODIMP nsOutlookSettings::SetLocation(nsIFile *location)
{
  return( NS_OK);
}

NS_IMETHODIMP nsOutlookSettings::Import(nsIMsgAccount **localMailAccount, bool *_retval)
{
  NS_PRECONDITION( _retval != nsnull, "null ptr");

  if (OutlookSettings::DoImport( localMailAccount)) {
    *_retval = true;
    IMPORT_LOG0( "Settings import appears successful\n");
  }
  else {
    *_retval = false;
    IMPORT_LOG0( "Settings import returned FALSE\n");
  }

  return( NS_OK);
}


HKEY OutlookSettings::FindAccountsKey( void)
{
  HKEY  sKey;
  if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Office\\Outlook\\OMI Account Manager\\Accounts", 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &sKey) == ERROR_SUCCESS) {
    return( sKey);
  }
  if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Office\\8.0\\Outlook\\OMI Account Manager\\Accounts", 0, KEY_QUERY_VALUE | KEY_ENUMERATE_SUB_KEYS, &sKey) == ERROR_SUCCESS) {
    return( sKey);
  }

  return( nsnull);
}

bool OutlookSettings::DoImport( nsIMsgAccount **ppAccount)
{
  HKEY  hKey = FindAccountsKey();
  if (hKey == nsnull) {
    IMPORT_LOG0( "*** Error finding Outlook registry account keys\n");
    return( false);
  }

  nsresult  rv;

  nsCOMPtr<nsIMsgAccountManager> accMgr =
           do_GetService(NS_MSGACCOUNTMANAGER_CONTRACTID, &rv);
    if (NS_FAILED(rv)) {
    IMPORT_LOG0( "*** Failed to create a account manager!\n");
    return( false);
  }

  HKEY    subKey = NULL;
  nsCString  defMailName;

  if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Office\\Outlook\\OMI Account Manager", 0, KEY_QUERY_VALUE, &subKey) != ERROR_SUCCESS)
    if (::RegOpenKeyEx( HKEY_CURRENT_USER, "Software\\Microsoft\\Office\\8.0\\Outlook\\OMI Account Manager", 0, KEY_QUERY_VALUE, &subKey) != ERROR_SUCCESS)
      subKey = NULL;

  if (subKey != NULL) {
    // First let's get the default mail account key name
    BYTE *  pBytes = nsOutlookRegUtil::GetValueBytes( subKey, "Default Mail Account");
    ::RegCloseKey( subKey);
    if (pBytes) {
      defMailName = (const char *)pBytes;
      nsOutlookRegUtil::FreeValueBytes( pBytes);
    }
  }

  // Iterate the accounts looking for POP3 & IMAP accounts...
  // Ignore LDAP & NNTP for now!
  DWORD    index = 0;
  DWORD    numChars;
  TCHAR    keyName[256];
  FILETIME  modTime;
  LONG    result = ERROR_SUCCESS;
  BYTE *    pBytes;
  int      popCount = 0;
  int      accounts = 0;
  nsCString  keyComp;

  while (result == ERROR_SUCCESS) {
    numChars = 256;
    result = ::RegEnumKeyEx( hKey, index, keyName, &numChars, NULL, NULL, NULL, &modTime);
    index++;
    if (result == ERROR_SUCCESS) {
      if (::RegOpenKeyEx( hKey, keyName, 0, KEY_QUERY_VALUE, &subKey) == ERROR_SUCCESS) {
        // Get the values for this account.
        IMPORT_LOG1( "Opened Outlook account: %s\n", (char *)keyName);

        nsIMsgAccount  *anAccount = nsnull;
        pBytes = nsOutlookRegUtil::GetValueBytes( subKey, "IMAP Server");
        if (pBytes) {
          if (DoIMAPServer( accMgr, subKey, (char *)pBytes, &anAccount))
            accounts++;
          nsOutlookRegUtil::FreeValueBytes( pBytes);
        }

        pBytes = nsOutlookRegUtil::GetValueBytes( subKey, "POP3 Server");
        if (pBytes) {
          if (popCount == 0) {
            if (DoPOP3Server( accMgr, subKey, (char *)pBytes, &anAccount)) {
              popCount++;
              accounts++;
              if (ppAccount && anAccount) {
                *ppAccount = anAccount;
                NS_ADDREF( anAccount);
              }
            }
          }
          else {
            if (DoPOP3Server( accMgr, subKey, (char *)pBytes, &anAccount)) {
              popCount++;
              accounts++;
              // If we created a mail account, get rid of it since
              // we have 2 POP accounts!
              if (ppAccount && *ppAccount) {
                NS_RELEASE( *ppAccount);
                *ppAccount = nsnull;
              }
            }
          }
          nsOutlookRegUtil::FreeValueBytes( pBytes);
        }

        if (anAccount) {
          // Is this the default account?
          keyComp = keyName;
          if (keyComp.Equals( defMailName)) {
            accMgr->SetDefaultAccount( anAccount);
          }
          NS_RELEASE( anAccount);
        }

        ::RegCloseKey( subKey);
      }
    }
  }

  // Now save the new acct info to pref file.
  rv = accMgr->SaveAccountInfo();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Can't save account info to pref file");

  return( accounts != 0);
}

nsresult OutlookSettings::GetAccountName(HKEY hKey, char *defaultName, nsString &acctName)
{
  BYTE *pAccName = nsOutlookRegUtil::GetValueBytes( hKey, "Account Name");
  nsresult rv = NS_OK;
  if (pAccName) {
     rv = nsMsgI18NConvertToUnicode(nsMsgI18NFileSystemCharset(),
                                    nsDependentCString(defaultName), acctName);
    nsOutlookRegUtil::FreeValueBytes( pAccName);
  }
  else
    acctName.AssignASCII(defaultName);
  return rv;
}

bool OutlookSettings::DoIMAPServer( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount)
{
  if (ppAccount)
    *ppAccount = nsnull;

  BYTE *pBytes;
  pBytes = nsOutlookRegUtil::GetValueBytes( hKey, "IMAP User Name");
  if (!pBytes)
    return( false);

  bool    result = false;

  // I now have a user name/server name pair, find out if it already exists?
  nsCOMPtr<nsIMsgIncomingServer> in;
  nsresult rv = pMgr->FindServer( nsDependentCString((const char *)pBytes), nsDependentCString(pServerName), NS_LITERAL_CSTRING("imap"), getter_AddRefs( in));
  if (NS_FAILED( rv) || (in == nsnull)) {
    // Create the incoming server and an account for it?
    rv = pMgr->CreateIncomingServer( nsDependentCString((const char *)pBytes), nsDependentCString(pServerName), NS_LITERAL_CSTRING("imap"), getter_AddRefs( in));
    if (NS_SUCCEEDED( rv) && in) {
      rv = in->SetType(NS_LITERAL_CSTRING("imap"));
      // TODO SSL, auth method

      IMPORT_LOG2( "Created IMAP server named: %s, userName: %s\n", pServerName, (char *)pBytes);

      nsString prettyName;
      if (NS_SUCCEEDED(GetAccountName(hKey, pServerName, prettyName)))
        rv = in->SetPrettyName( prettyName);
      // We have a server, create an account.
      nsCOMPtr<nsIMsgAccount>  account;
      rv = pMgr->CreateAccount( getter_AddRefs( account));
      if (NS_SUCCEEDED( rv) && account) {
        rv = account->SetIncomingServer( in);

        IMPORT_LOG0( "Created an account and set the IMAP server as the incoming server\n");

        // Fiddle with the identities
        SetIdentities( pMgr, account, hKey);
        result = true;
        if (ppAccount)
          account->QueryInterface( NS_GET_IID(nsIMsgAccount), (void **)ppAccount);
      }
    }
  }
  else
    result = true;

  nsOutlookRegUtil::FreeValueBytes( pBytes);

  return( result);
}

bool OutlookSettings::DoPOP3Server( nsIMsgAccountManager *pMgr, HKEY hKey, char *pServerName, nsIMsgAccount **ppAccount)
{
  if (ppAccount)
    *ppAccount = nsnull;

  BYTE *pBytes;
  pBytes = nsOutlookRegUtil::GetValueBytes( hKey, "POP3 User Name");
  if (!pBytes)
    return( false);

  bool result = false;

  // I now have a user name/server name pair, find out if it already exists?
  nsCOMPtr<nsIMsgIncomingServer> in;
  nsresult rv = pMgr->FindServer( nsDependentCString((const char *)pBytes), nsDependentCString(pServerName), NS_LITERAL_CSTRING("pop3"), getter_AddRefs( in));
  if (NS_FAILED( rv) || (in == nsnull)) {
    // Create the incoming server and an account for it?
    rv = pMgr->CreateIncomingServer(nsDependentCString((const char *)pBytes), nsDependentCString(pServerName), NS_LITERAL_CSTRING("pop3"), getter_AddRefs( in));
    if (NS_SUCCEEDED( rv) && in) {
      rv = in->SetType(NS_LITERAL_CSTRING("pop3"));

      // TODO SSL, auth method

        nsCOMPtr<nsIPop3IncomingServer> pop3Server = do_QueryInterface(in);
        if (pop3Server) {
            // set local folders as the Inbox to use for this POP3 server
            nsCOMPtr<nsIMsgIncomingServer> localFoldersServer;
            pMgr->GetLocalFoldersServer(getter_AddRefs(localFoldersServer));

            if (!localFoldersServer)
            {
                // XXX: We may need to move this local folder creation code to the generic nsImportSettings code
                // if the other import modules end up needing to do this too.
                // if Local Folders does not exist already, create it
                rv = pMgr->CreateLocalMailAccount();
                if (NS_FAILED(rv)) {
                    IMPORT_LOG0( "*** Failed to create Local Folders!\n");
                    return false;
                }
                pMgr->GetLocalFoldersServer(getter_AddRefs(localFoldersServer));
            }

            // now get the account for this server
            nsCOMPtr<nsIMsgAccount> localFoldersAccount;
            pMgr->FindAccountForServer(localFoldersServer, getter_AddRefs(localFoldersAccount));
            if (localFoldersAccount)
            {
              nsCString localFoldersAcctKey;
              localFoldersAccount->GetKey(localFoldersAcctKey);
              pop3Server->SetDeferredToAccount(localFoldersAcctKey);
              pop3Server->SetDeferGetNewMail(true);
            }
        }

        IMPORT_LOG2( "Created POP3 server named: %s, userName: %s\n", pServerName, (char *)pBytes);

        nsString prettyName;
        if (NS_SUCCEEDED(GetAccountName(hKey, pServerName, prettyName)))
          rv = in->SetPrettyName( prettyName);
      // We have a server, create an account.
      nsCOMPtr<nsIMsgAccount>  account;
      rv = pMgr->CreateAccount( getter_AddRefs( account));
      if (NS_SUCCEEDED( rv) && account) {
        rv = account->SetIncomingServer( in);

      IMPORT_LOG0( "Created a new account and set the incoming server to the POP3 server.\n");

        nsCOMPtr<nsIPop3IncomingServer> pop3Server = do_QueryInterface(in, &rv);
        NS_ENSURE_SUCCESS(rv,rv);
        BYTE *pLeaveOnServer = nsOutlookRegUtil::GetValueBytes( hKey, "Leave Mail On Server");
        if (pLeaveOnServer)
        {
          pop3Server->SetLeaveMessagesOnServer(*pLeaveOnServer == 1 ? true : false);
          nsOutlookRegUtil::FreeValueBytes(pLeaveOnServer);
        }

        // Fiddle with the identities
        SetIdentities( pMgr, account, hKey);
        result = true;
        if (ppAccount)
          account->QueryInterface( NS_GET_IID(nsIMsgAccount), (void **)ppAccount);
      }
    }
  }
  else
    result = true;

  nsOutlookRegUtil::FreeValueBytes( pBytes);

  return( result);
}

bool OutlookSettings::IdentityMatches( nsIMsgIdentity *pIdent, const char *pName, const char *pServer, const char *pEmail, const char *pReply, const char *pUserName)
{
  if (!pIdent)
    return( false);

  char *  pIName = nsnull;
  nsCString pIEmail;
  nsCString pIReply;

  bool    result = true;

  // The test here is:
  // If the smtp host is the same
  //  and the email address is the same (if it is supplied)
  //  and the reply to address is the same (if it is supplied)
  //  then we match regardless of the full name.


  nsresult rv;
  rv = pIdent->GetEmail(pIEmail);
  rv = pIdent->GetReplyTo(pIReply);


  // for now, if it's the same server and reply to and email then it matches
  if (pReply && !pIReply.Equals(pReply, nsCaseInsensitiveCStringComparator()))
    result = false;
  if (pEmail && !pIEmail.Equals(pEmail, nsCaseInsensitiveCStringComparator()))
    result = false;

  return( result);
}

void OutlookSettings::SetIdentities( nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, HKEY hKey)
{
  // Get the relevant information for an identity
  char *pName = (char *)nsOutlookRegUtil::GetValueBytes( hKey, "SMTP Display Name");
  char *pServer = (char *)nsOutlookRegUtil::GetValueBytes( hKey, "SMTP Server");
  char *pEmail = (char *)nsOutlookRegUtil::GetValueBytes( hKey, "SMTP Email Address");
  char *pReply = (char *)nsOutlookRegUtil::GetValueBytes( hKey, "SMTP Reply To Email Address");
  nsCString userName;
  userName.Adopt((char *)nsOutlookRegUtil::GetValueBytes( hKey, "SMTP User Name"));
  char *pOrgName = (char *)nsOutlookRegUtil::GetValueBytes( hKey, "SMTP Organization Name");

  nsresult rv;

  if (pEmail && pName && pServer) {
    // The default identity, nor any other identities matched,
    // create a new one and add it to the account.
    nsCOMPtr<nsIMsgIdentity>  id;
    rv = pMgr->CreateIdentity( getter_AddRefs( id));
    if (id) {
      nsAutoString name, organization;
      rv = nsMsgI18NConvertToUnicode(nsMsgI18NFileSystemCharset(),
        nsDependentCString(pName), name);
      if (NS_SUCCEEDED(rv))
      {
        id->SetFullName(name);
        id->SetIdentityName(name);
      }

      if (pOrgName) {
        rv = nsMsgI18NConvertToUnicode(nsMsgI18NFileSystemCharset(),
          nsDependentCString(pOrgName), organization);
        if (NS_SUCCEEDED(rv))
          id->SetOrganization(organization);
      }

      id->SetEmail(nsDependentCString(pEmail));
      if (pReply)
        id->SetReplyTo(nsDependentCString(pReply));
      pAcc->AddIdentity( id);

      IMPORT_LOG0( "Created identity and added to the account\n");
      IMPORT_LOG1( "\tname: %s\n", pName);
      IMPORT_LOG1( "\temail: %s\n", pEmail);
    }
  }

  if (userName.IsEmpty()) {
    nsCOMPtr <nsIMsgIncomingServer>  incomingServer;
    rv = pAcc->GetIncomingServer(getter_AddRefs( incomingServer));
    if (NS_SUCCEEDED(rv) && incomingServer)
      rv = incomingServer->GetUsername(userName);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to get UserName from incomingServer");
  }

  SetSmtpServer( pMgr, pAcc, pServer, userName);

  nsOutlookRegUtil::FreeValueBytes( (BYTE *)pName);
  nsOutlookRegUtil::FreeValueBytes( (BYTE *)pServer);
  nsOutlookRegUtil::FreeValueBytes( (BYTE *)pEmail);
  nsOutlookRegUtil::FreeValueBytes( (BYTE *)pReply);
}

void OutlookSettings::SetSmtpServer(nsIMsgAccountManager *pMgr, nsIMsgAccount *pAcc, char *pServer,
                                    const nsCString& user)
{
  nsresult rv;
  nsCOMPtr<nsISmtpService> smtpService(do_GetService(NS_SMTPSERVICE_CONTRACTID, &rv));
  if (NS_SUCCEEDED(rv) && smtpService) {
    nsCOMPtr<nsISmtpServer> foundServer;
    rv = smtpService->FindServer( user.get(), pServer, getter_AddRefs( foundServer));
    if (NS_SUCCEEDED( rv) && foundServer) {
      IMPORT_LOG1( "SMTP server already exists: %s\n", pServer);
      return;
    }
    nsCOMPtr<nsISmtpServer> smtpServer;
    rv = smtpService->CreateSmtpServer( getter_AddRefs( smtpServer));
    if (NS_SUCCEEDED( rv) && smtpServer) {
      smtpServer->SetHostname(nsDependentCString(pServer));
      if (!user.IsEmpty())
        smtpServer->SetUsername(user);
      // TODO SSL, auth method
      IMPORT_LOG1( "Ceated new SMTP server: %s\n", pServer);
    }
  }
}

