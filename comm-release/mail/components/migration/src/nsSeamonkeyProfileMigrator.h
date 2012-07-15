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
 * The Original Code is The Browser Profile Migrator.
 *
 * The Initial Developer of the Original Code is Ben Goodger.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Ben Goodger <ben@bengoodger.com>
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

#ifndef seamonkeyprofilemigrator___h___
#define seamonkeyprofilemigrator___h___

#include "nsIMailProfileMigrator.h"
#include "nsIMutableArray.h"
#include "nsNetscapeProfileMigratorBase.h"

class nsIPrefBranch;
class nsIPrefService;

class nsSeamonkeyProfileMigrator : public nsNetscapeProfileMigratorBase
{
public:
  NS_DECL_ISUPPORTS

  nsSeamonkeyProfileMigrator();
  virtual ~nsSeamonkeyProfileMigrator();

  // nsIMailProfileMigrator methods
  NS_IMETHOD Migrate(PRUint16 aItems, nsIProfileStartup* aStartup,
                        const PRUnichar* aProfile);
  NS_IMETHOD GetMigrateData(const PRUnichar* aProfile, bool aReplace,
                            PRUint16* aResult);
  NS_IMETHOD GetSourceProfiles(nsIArray** aResult);

protected:
  nsresult FillProfileDataFromSeamonkeyRegistry();
  nsresult GetSourceProfile(const PRUnichar* aProfile);

  nsresult CopyPreferences(bool aReplace);
  nsresult TransformPreferences(const nsAString& aSourcePrefFileName,
                                const nsAString& aTargetPrefFileName);

  nsresult DummyCopyRoutine(bool aReplace);
  nsresult CopyJunkTraining(bool aReplace);  
  nsresult CopyPasswords(bool aReplace);
  nsresult CopyMailFolders(PBStructArray &aMailServers,
                           nsIPrefService* aPrefBranch);
  nsresult CopyAddressBookDirectories(PBStructArray &aLdapServers,
                                      nsIPrefService* aPrefService);
  nsresult CopySignatureFiles(PBStructArray &aIdentities,
                              nsIPrefService* aPrefBranch);

  void ReadBranch(const char *branchName,  nsIPrefService *aPrefService,
                  PBStructArray &aPrefs);
  void WriteBranch(const char *branchName, nsIPrefService *aPrefService,
                   PBStructArray &aPrefs);

private:
  nsCOMPtr<nsIMutableArray> mProfileNames;
  nsCOMPtr<nsIMutableArray> mProfileLocations;
};
 
#endif
