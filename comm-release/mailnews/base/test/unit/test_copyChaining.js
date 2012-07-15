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
 * David Bienvenu <bienvenu@nventure.com>.
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

// Test of chaining copies between the same folders

const copyService = Cc["@mozilla.org/messenger/messagecopyservice;1"]
                      .getService(Ci.nsIMsgCopyService);

load("../../../resources/messageGenerator.js");

var gCopySource;
var gCopyDest;
var gMsgEnumerator;
var gCurTestNum = 1;

// main test

var hdrs = [];

const gTestArray =
[
  function copyMsg1() {
    gMsgEnumerator = gCopySource.msgDatabase.EnumerateMessages();
    CopyNextMessage();
  },
  function copyMsg2() {
    CopyNextMessage();
  },
  function copyMsg3() {
    CopyNextMessage();
  },
  function copyMsg4() {
    CopyNextMessage();
  },
];

function CopyNextMessage()
{
  if (gMsgEnumerator.hasMoreElements()) {
    let msgHdr = gMsgEnumerator.getNext().QueryInterface(
      Components.interfaces.nsIMsgDBHdr);
    var messages = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    messages.appendElement(msgHdr, false);
    copyService.CopyMessages(gCopySource, messages, gCopyDest, true,
                             copyListener, null, false);
  }
  else
    do_throw ('TEST FAILED - out of messages');
}

function run_test()
{

  loadLocalMailAccount();
  let messageGenerator = new MessageGenerator();
  let scenarioFactory = new MessageScenarioFactory(messageGenerator);

  // "Master" do_test_pending(), paired with a do_test_finished() at the end of
  // all the operations.
  do_test_pending();

  gCopyDest = gLocalInboxFolder.createLocalSubfolder("copyDest");
  // build up a diverse list of messages
  let messages = [];
  messages = messages.concat(scenarioFactory.directReply(10));
  gCopySource = gLocalIncomingServer.rootMsgFolder.createLocalSubfolder("copySource");
  addMessagesToFolder(messages, gCopySource);

  updateFolderAndNotify(gCopySource, doTest);
  return true;
}

function doTest()
{
  var test = gCurTestNum;
  if (test <= gTestArray.length)
  {
    var testFn = gTestArray[test-1];
    dump("Doing test " + test + " " + testFn.name + "\n");

    try {
      testFn();
    } catch(ex) {
      do_throw ('TEST FAILED ' + ex);
    }
  }
  else
    endTest();
}

function endTest()
{
  // Cleanup, null out everything
  dump(" Exiting mail tests\n");
  gMsgEnumerator = null;
  do_test_finished(); // for the one in run_test()
}

// nsIMsgCopyServiceListener implementation
var copyListener = 
{
  OnStartCopy: function() {},
  OnProgress: function(aProgress, aProgressMax) {},
  SetMessageKey: function(aKey) {},
  SetMessageId: function(aMessageId) {},
  OnStopCopy: function(aStatus)
  {
    // Check: message successfully copied.
    do_check_eq(aStatus, 0);
    ++gCurTestNum;
    doTest();
  }
};

