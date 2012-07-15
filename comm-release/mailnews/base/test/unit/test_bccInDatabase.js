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
 * Kent James <kent@caspia.com>.
 * Portions created by the Initial Developer are Copyright (C) 2009
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
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

/*
 * Testing of bcc in message summary file added in bug 481667
 */

const copyService = Cc["@mozilla.org/messenger/messagecopyservice;1"]
                      .getService(Ci.nsIMsgCopyService);

var hdr;

function run_test()
{
  loadLocalMailAccount();

  var copyListener = 
  {
    OnStartCopy: function() {},
    OnProgress: function(aProgress, aProgressMax) {},
    SetMessageKey: function(aKey) { hdr = gLocalInboxFolder.GetMessageHeader(aKey);},
    SetMessageId: function(aMessageId) {},
    OnStopCopy: function(aStatus) { continueTest();}
  };

  // Get a message into the local filestore.
  var draft = do_get_file("../../../data/draft1");
  do_test_pending();
  copyService.CopyFileMessage(draft, gLocalInboxFolder, null, false, 0,
                              "", copyListener, null);
}

function continueTest()
{
  //dump("\nbccList >" + hdr.bccList);
  //dump("\nccList >" + hdr.ccList);
  //dump("\n");
  do_check_true(hdr.bccList.indexOf("Another Person") >= 0);
  do_check_true(hdr.bccList.indexOf("<u1@example.com>") >= 0);
  do_check_false(hdr.bccList.indexOf("IDoNotExist") >=0);
  hdr = null;
  do_test_finished();
}

