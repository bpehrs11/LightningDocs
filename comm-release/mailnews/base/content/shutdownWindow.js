/*
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
#   Nick Kreeger <nick.kreeger@park.edu>
#
# Portions created by the Initial Developer are Copyright (C) 2007
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****
*/

var curTaskIndex = 0;
var numTasks = 0;
var stringBundle;

var msgShutdownService = Components.classes["@mozilla.org/messenger/msgshutdownservice;1"]
                           .getService(Components.interfaces.nsIMsgShutdownService);

function onLoad()
{
  numTasks = msgShutdownService.getNumTasks();

  stringBundle = document.getElementById("bundle_shutdown");
  document.title = stringBundle.getString("shutdownDialogTitle");

  updateTaskProgressLabel(1);
  updateProgressMeter(0);

  msgShutdownService.startShutdownTasks();
}

function updateProgressLabel(inTaskName)
{
  var curTaskLabel = document.getElementById("shutdownStatus_label");
  curTaskLabel.value = inTaskName;
}

function updateTaskProgressLabel(inCurTaskNum)
{
  var taskProgressLabel = document.getElementById("shutdownTask_label");
  taskProgressLabel.value = stringBundle.getFormattedString("taskProgress", [inCurTaskNum, numTasks]);
}

function updateProgressMeter(inPercent)
{
  var taskProgressmeter = document.getElementById('shutdown_progressmeter');
  taskProgressmeter.value = inPercent;
}

function onCancel()
{
  msgShutdownService.cancelShutdownTasks();
}

function nsMsgShutdownTaskListener() 
{
  msgShutdownService.setShutdownListener(this);
}

nsMsgShutdownTaskListener.prototype = 
{
  QueryInterface : function(iid)
  {
    if (iid.equals(Components.interfaces.nsIWebProgressListener) ||
        iid.equals(Components.interfaces.nsISupportsWeakReference) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;

    throw Components.results.NS_NOINTERFACE;
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
    {
      window.close();
    }
  },

  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
  {
    updateProgressMeter(((aCurTotalProgress / aMaxTotalProgress) * 100));
    updateTaskProgressLabel(aCurTotalProgress + 1);
  },

  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags)
  {
    // we can ignore this notification
  },

  onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage)
  {
    if (aMessage)
      updateProgressLabel(aMessage);
  },

  onSecurityChange: function(aWebProgress, aRequest, state)
  {
    // we can ignore this notification
  }
}

var MsgShutdownTaskListener = new nsMsgShutdownTaskListener();

