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
 * The Original Code is Instantbird.
 *
 * The Initial Developer of the Original Code is
 * Florian Quèze <florian@queze.net>.
 * Portions created by the Initial Developer are Copyright (C) 2011
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

const {interfaces: Ci, utils: Cu} = Components;

Cu.import("resource:///modules/imXPCOMUtils.jsm");
Cu.import("resource:///modules/jsProtoHelper.jsm");
Cu.import("resource:///modules/xmpp.jsm");
Cu.import("resource:///modules/xmpp-session.jsm");

XPCOMUtils.defineLazyGetter(this, "_", function()
  l10nHelper("chrome://chat/locale/facebook.properties")
);

function FacebookAccount(aProtoInstance, aImAccount) {
  this._init(aProtoInstance, aImAccount);
}
FacebookAccount.prototype = {
  __proto__: XMPPAccountPrototype,
  get canJoinChat() false,
  connect: function() {
    if (this.name.indexOf("@") == -1) {
      let jid = this.name + "@chat.facebook.com/" + XMPPDefaultResource;
      this._jid = this._parseJID(jid);
    }
    else {
      this._jid = this._parseJID(this.name);
      if (this._jid.domain != "chat.facebook.com") {
        // We can't use this.onError because this._connection doesn't exist.
        this.reportDisconnecting(Ci.prplIAccount.ERROR_INVALID_USERNAME,
                                 _("connection.error.useUsernameNotEmailAddress"));
        this.reportDisconnected();
        return;
      }
    }

    this._connection = new XMPPSession("chat.facebook.com", 5222,
                                       "opportunistic_tls", this._jid,
                                       this.imAccount.password, this);
  }
};

function FacebookProtocol() {
}
FacebookProtocol.prototype = {
  __proto__: GenericProtocolPrototype,
  get normalizedName() "facebook",
  get name() "Facebook Chat",
  get iconBaseURI() "chrome://prpl-facebook/skin/",
  getAccount: function(aImAccount) new FacebookAccount(this, aImAccount),
  classID: Components.ID("{1d1d0bc5-610c-472f-b2cb-4b89857d80dc}")
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([FacebookProtocol]);
