/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * The Original Code is Mozilla Communicator client code, released
 * March 31, 1998.
 *
 * The Initial Developer of the Original Code is
 * Netscape Communications Corporation.
 * Portions created by the Initial Developer are Copyright (C) 1998
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   William A. ("PowerGUI") Law <law@netscape.com>
 *   Scott MacGregor <mscott@netscape.com>
 *   jean-Francois Ducarroz <ducarroz@netscape.com>
 *   Žiga Sancin <bisi@pikslar.com>
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

var nsIMsgCompDeliverMode = Components.interfaces.nsIMsgCompDeliverMode;

// dialog is just an array we'll use to store various properties from the dialog document...
var dialog;

// the msgProgress is a nsIMsgProgress object
var msgProgress = null; 

// random global variables...
var itsASaveOperation = false;
var gSendProgressStringBundle;

// all progress notifications are done through the nsIWebProgressListener implementation...
var progressListener = {
    onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
    {
      if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_START)
      {
        // Put progress meter in undetermined mode.
        dialog.progress.setAttribute("mode", "undetermined");
      }
      
      if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
      {
        // we are done sending/saving the message...
        // Indicate completion in status area.
        var msg;
        if (itsASaveOperation)
          msg = gSendProgressStringBundle.getString("messageSaved");
        else
          msg = gSendProgressStringBundle.getString("messageSent");
        dialog.status.setAttribute("value", msg);

        // Put progress meter at 100%.
        dialog.progress.setAttribute("value", 100);
        dialog.progress.setAttribute("mode", "normal");
        var percentMsg = gSendProgressStringBundle.getFormattedString("percentMsg", [100]);
        dialog.progressText.setAttribute("value", percentMsg);

        window.close();
      }
    },
    
    onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
    {
      // Calculate percentage.
      var percent;
      if (aMaxTotalProgress > 0)
      {
        percent = Math.round(aCurTotalProgress / aMaxTotalProgress * 100);
        if (percent > 100)
          percent = 100;
        
        dialog.progress.removeAttribute("mode");
        
        // Advance progress meter.
        dialog.progress.setAttribute("value", percent);

        // Update percentage label on progress meter.
        var percentMsg = gSendProgressStringBundle.getFormattedString("percentMsg", [percent]);
        dialog.progressText.setAttribute("value", percentMsg);
      } 
      else 
      {
        // Progress meter should be barber-pole in this case.
        dialog.progress.setAttribute("mode", "undetermined");
        // Update percentage label on progress meter.
        dialog.progressText.setAttribute("value", "");
      }
    },

	  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags)
    {
      // we can ignore this notification
    },

    onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage)
    {
      if (aMessage != "")
        dialog.status.setAttribute("value", aMessage);
    },

    onSecurityChange: function(aWebProgress, aRequest, state)
    {
      // we can ignore this notification
    },

    QueryInterface : function(iid)
    {
      if (iid.equals(Components.interfaces.nsIWebProgressListener) ||
          iid.equals(Components.interfaces.nsISupportsWeakReference) ||
          iid.equals(Components.interfaces.nsISupports))
        return this;
     
      throw Components.results.NS_NOINTERFACE;
    }
};

function onLoad()
{
    // Set global variables.
    var subject = "";
    gSendProgressStringBundle = document.getElementById("sendProgressStringBundle");
    msgProgress = window.arguments[0];
    if (window.arguments[1])
    {
      var progressParams = window.arguments[1].QueryInterface(Components.interfaces.nsIMsgComposeProgressParams)
      if (progressParams)
      {
        itsASaveOperation = (progressParams.deliveryMode != nsIMsgCompDeliverMode.Now);
        subject = progressParams.subject;
      }
    }

    if (!msgProgress)
    {
      dump("Invalid argument to sendProgress.xul\n");
      window.close()
      return;
    }

    dialog = {};
    dialog.status       = document.getElementById("dialog.status");
    dialog.progress     = document.getElementById("dialog.progress");
    dialog.progressText = document.getElementById("dialog.progressText");

    // set our web progress listener on the helper app launcher
    msgProgress.registerListener(progressListener);

    var prefix = itsASaveOperation ? "titlePrefixSave" : "titlePrefixSend";
    document.title = gSendProgressStringBundle.getString(prefix) + " " + subject;
}

function onUnload() 
{
  if (msgProgress)
  {
    try
    {
      msgProgress.unregisterListener(progressListener);
      msgProgress = null;
    } catch (e) {}
  }
}

// If the user presses cancel, tell the app launcher and close the dialog...
function onCancel () 
{
  // Cancel app launcher.
  try
  {
    msgProgress.processCanceledByUser = true;
  } catch (e)
  {
    return true;
  }
    
  // don't Close up dialog by returning false, the backend will close the dialog when everything will be aborted.
  return false;
}
