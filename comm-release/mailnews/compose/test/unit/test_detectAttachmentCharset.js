/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 * Test suite for auto-detecting attachment file charset.
 */

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource:///modules/mailServices.js");

var gSmtpServer;
var gDraftFolder;
var gCurTestNum = 0;

var progressListener = {
  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
      do_timeout(0, function() { doTest(++gCurTestNum); });
    }
  },

  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress,
    aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress) {},
  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags) {},
  onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage) {},
  onSecurityChange: function(aWebProgress, aRequest, state) {},

  QueryInterface : function(iid) {
    if (iid.equals(Ci.nsIWebProgressListener) ||
        iid.equals(Ci.nsISupportsWeakReference) ||
        iid.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_NOINTERFACE;
  }
};

function createMessage(attachmentPath)
{
  Services.prefs.setIntPref("mail.strictly_mime.parm_folding", 0);

  var fields = Cc["@mozilla.org/messengercompose/composefields;1"]
                 .createInstance(Ci.nsIMsgCompFields);
  var params = Cc["@mozilla.org/messengercompose/composeparams;1"]
                 .createInstance(Ci.nsIMsgComposeParams);
  params.composeFields = fields;

  var msgCompose = MailServices.compose.initCompose(params);
  var identity = getSmtpIdentity(null, gSmtpServer);

  var rootFolder = gLocalIncomingServer.rootMsgFolder;
  gDraftFolder = null;
  // Make sure the drafts folder is empty
  try {
    gDraftFolder = rootFolder.getChildNamed("Drafts");
    // try to delete
    rootFolder.propagateDelete(gDraftFolder, true, null);
  } catch (e) {
    // we don't have to remove the folder because it doen't exist yet
  }
  // Create a new, empty drafts folder
  gDraftFolder = rootFolder.createLocalSubfolder("Drafts");

  var attachment = Cc["@mozilla.org/messengercompose/attachment;1"]
                     .createInstance(Ci.nsIMsgAttachment);
  //Set attachment file
  var file = do_get_file(attachmentPath);
  attachment.url = "file://" + file.path;
  attachment.contentType = 'text/plain';
  attachment.name = file.leafName;
  fields.addAttachment(attachment);

  var progress = Cc["@mozilla.org/messenger/progress;1"]
                   .createInstance(Ci.nsIMsgProgress);
  progress.registerListener(progressListener);
  msgCompose.SendMsg(Ci.nsIMsgSend.nsMsgSaveAsDraft, identity, "", null,
                     progress);
}

function checkAttachment(charset)
{
  var msgData = getContentFromMessage(firstMsgHdr(gDraftFolder));
  var pos = msgData.indexOf("Content-Type: text/plain; charset=" + charset + ";");
  do_check_neq(pos, -1);
  do_timeout(0, function() {doTest(++gCurTestNum);});
}

// get the first message header found in a folder
function firstMsgHdr(folder)
{
  let enumerator = folder.msgDatabase.EnumerateMessages();
  if (enumerator.hasMoreElements())
    return enumerator.getNext().QueryInterface(Ci.nsIMsgDBHdr);
  return null;
}

/*
 * Get the full message content from a local folder.
 *
 * aMsgHdr: nsIMsgDBHdr object whose text body will be read
 *          returns: string with full message contents
 */
function getContentFromMessage(aMsgHdr)
{
  const MAX_MESSAGE_LENGTH = 65536;
  let msgFolder = aMsgHdr.folder;
  let msgUri = msgFolder.getUriForMsg(aMsgHdr);

  let messenger = Cc["@mozilla.org/messenger;1"]
                    .createInstance(Ci.nsIMessenger);
  let streamListener = Cc["@mozilla.org/network/sync-stream-listener;1"]
                         .createInstance(Ci.nsISyncStreamListener);
  messenger.messageServiceFromURI(msgUri).streamMessage(msgUri,
                                                        streamListener,
                                                        null,
                                                        null,
                                                        false,
                                                        "",
                                                        false);
  let sis = Cc["@mozilla.org/scriptableinputstream;1"]
              .createInstance(Ci.nsIScriptableInputStream);
  sis.init(streamListener.inputStream);
  return sis.read(MAX_MESSAGE_LENGTH);
}

const gTestArray =
[
  function createMessage1() { createMessage("data/test-UTF-8.txt"); },
  function checkAttachment1() { checkAttachment("UTF-8"); },

  function createMessage2() { createMessage("data/test-UTF-16.txt"); },
  function checkAttachment2() { checkAttachment("UTF-16"); },

  function createMessage3() { createMessage("data/test-SHIFT_JIS.txt"); },
  function checkAttachment3() { checkAttachment("Shift_JIS"); }

]

function run_test()
{
  // Ensure we have at least one mail account
  loadLocalMailAccount();

  gSmtpServer = getBasicSmtpServer();

  do_test_pending();

  doTest(1);
}

function doTest(test)
{
  dump("doTest " + test + "\n");
  if (test <= gTestArray.length) {
    gCurTestNum = test;
 
    var testFn = gTestArray[test-1];

    // Set a limit in case the notifications haven't arrived (i.e. a problem)
    do_timeout(10000, function()
    {
      if (gCurTestNum == test)
        do_throw(
          "Notifications not received in 10000 ms for operation " +
          testFn.name);
    });
    try {
      testFn();
    } catch(ex) {
      dump(ex);
      do_throw(ex);
    }
  }
  else {
    do_test_finished(); // for the one in run_test()
  }
}
