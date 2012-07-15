# -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
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
# The Original Code is an RSS Feed Item
#
# The Initial Developer of the Original Code is
# The Mozilla Foundation.
# Portions created by the Initial Developer are Copyright (C) 2004
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
# ***** END LICENSE BLOCK ***** */

// Handy conversion values.
const HOURS_TO_MINUTES = 60;
const MINUTES_TO_SECONDS = 60;
const SECONDS_TO_MILLISECONDS = 1000;
const MINUTES_TO_MILLISECONDS = MINUTES_TO_SECONDS * SECONDS_TO_MILLISECONDS;
const HOURS_TO_MILLISECONDS = HOURS_TO_MINUTES * MINUTES_TO_MILLISECONDS;
const ENCLOSURE_BOUNDARY_PREFIX = "--------------";  // 14 dashes
const ENCLOSURE_HEADER_BOUNDARY_PREFIX = "------------"; // 12 dashes

const MESSAGE_TEMPLATE = "\n\
<html>\n\
  <head>\n\
    <title>%TITLE%</title>\n\
    <base href=\"%BASE%\">\n\
  </head>\n\
  <body id=\"msgFeedSummaryBody\" selected=\"false\">\n\
    %CONTENT%\n\
  </body>\n\
</html>\n\
";

function FeedItem()
{
  this.mDate = new Date().toString();
  this.mUnicodeConverter = Components.classes["@mozilla.org/intl/scriptableunicodeconverter"]
                                     .createInstance(Components.interfaces.nsIScriptableUnicodeConverter);
}

FeedItem.prototype =
{
  // Currently only for IETF Atom. RSS2 with GUIDs should do this as too.
  isStoredWithId: false,
  // Only for IETF Atom
  xmlContentBase: null,
  id: null,
  feed: null,
  description: null,
  content: null,
  // Currently only support one enclosure per feed item...
  enclosure: null,
  // TO DO: this needs to be localized
  title: "(no subject)",
  author: "anonymous",
  mURL: null,
  characterSet: "",

  get url()
  {
    return this.mURL;
  },

  set url(aVal)
  {
    try
    {
      var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                                .getService(Components.interfaces.nsIIOService);
      this.mURL = ioService.newURI(aVal, null, null).spec;
    }
    catch(ex)
    {
      // The url as published or constructed can be a non url.  It's used as a
      // feeditem identifier in feeditems.rdf, as a messageId, and as an href
      // and for the content-base header.  Save as is; ensure not null.
      this.mURL = aVal ? aVal : "";
    }
  },

  get date()
  {
    return this.mDate;
  },

  set date (aVal)
  {
    this.mDate = aVal;
  },

  get identity ()
  {
    return this.feed.name + ": " + this.title + " (" + this.id + ")"
  },

  get messageID()
  {
    var messageID = this.id || this.mURL || this.title;

    debug('messageID - id = ' + this.id);
    debug('messageID - mURL = ' + this.mURL);
    debug('messageID - title = ' + this.title);
    debug('messageID - messageID = ' + messageID);

    // Escape occurrences of message ID meta characters <, >, and @.
    messageID.replace(/</g, "%3C");
    messageID.replace(/>/g, "%3E");
    messageID.replace(/@/g, "%40");
    messageID = messageID + "@" + "localhost.localdomain";
    return messageID;
  },

  get itemUniqueURI()
  {
    return (this.isStoredWithId && this.id) ? createURN(this.id) :
           createURN(this.mURL || this.id);
  },

  get contentBase()
  {
    if(this.xmlContentBase)
      return this.xmlContentBase
    else
      return this.mURL;
  },

  store: function()
  {
    this.mUnicodeConverter.charset = this.characterSet;

    // this.title and this.content contain HTML
    // this.mUrl and this.contentBase contain plain text

    let resource = this.findStoredResource();
    if (resource == null)
    {
      resource = rdf.GetResource(this.itemUniqueURI);
      if (!this.content)
      {
        debug(this.identity + " no content; storing");
        this.content = this.description || this.title;
      }

      debug(this.identity + " store both remote/no content and content items");
      var content = MESSAGE_TEMPLATE;
      content = content.replace(/%TITLE%/, this.title);
      content = content.replace(/%BASE%/, htmlEscape(this.contentBase));
      content = content.replace(/%CONTENT%/, this.content);
      // XXX store it elsewhere, f.e. this.page
      this.content = content;
      this.writeToFolder();
      this.markStored(resource);
    }
    this.markValid(resource);
  },

  findStoredResource: function()
  {
    // Checks to see if the item has already been stored in its feed's message folder.

    debug(this.identity + " checking to see if stored");

    var server = this.feed.server;
    var folder = this.feed.folder;

    if (!folder)
    {
      debug(this.feed.name + " folder doesn't exist; creating");
      debug("creating " + this.feed.name + "as child of " +
          server.rootMsgFolder + "\n");
      server.rootMsgFolder.createSubfolder(this.feed.name, null /* supposed to be a msg window */);
      folder = server.rootMsgFolder.findSubFolder(this.feed.name);
      debug(this.identity + " not stored (folder didn't exist)");
      return null;
    }

    var ds = getItemsDS(server);
    var itemURI = this.itemUniqueURI;
    var itemResource = rdf.GetResource(itemURI);

    var downloaded = ds.GetTarget(itemResource, FZ_STORED, true);

    // Backward compatibility: we might have stored this item before
    // isStoredWithId has been turned on for RSS 2.0 (bug 354345).
    // Check whether this item has been stored with its URL.
    if (!downloaded && this.mURL && itemURI != this.mURL)
    {
      itemResource = rdf.GetResource(this.mURL);
      downloaded = ds.GetTarget(itemResource, FZ_STORED, true);
    }

    // Backward compatibility: the item may have been stored
    // using the previous unique URI algorithm.
    // (bug 410842 & bug 461109)
    if (!downloaded)
    {
      itemResource = rdf.GetResource((this.isStoredWithId && this.id) ?
                                     ("urn:" + this.id) :
                                     (this.mURL || ("urn:" + this.id)));
      downloaded = ds.GetTarget(itemResource, FZ_STORED, true);
    }

    if (!downloaded ||
        downloaded.QueryInterface(Components.interfaces.nsIRDFLiteral)
                  .Value == "false")
    {
      // HACK ALERT: before we give up, try to work around an entity
      // escaping bug in RDF. See Bug #258465 for more details
      itemURI = itemURI.replace(/&lt;/g, '<');
      itemURI = itemURI.replace(/&gt;/g, '>');
      itemURI = itemURI.replace(/&quot;/g, '"');
      itemURI = itemURI.replace(/&amp;/g, '&');

      debug('Failed to find item, trying entity replacement version: '  + itemURI);
      itemResource = rdf.GetResource(itemURI);
      downloaded = ds.GetTarget(itemResource, FZ_STORED, true);

      if (downloaded)
      {
        debug(this.identity + " stored");
        return itemResource;
      }

      debug(this.identity + " not stored");
      return null;
    }
    else
    {
      debug(this.identity + " stored");
      return itemResource;
    }
  },

  markValid: function(resource)
  {
    var ds = getItemsDS(this.feed.server);

    var newTimeStamp = rdf.GetLiteral(new Date().getTime());
    var currentTimeStamp = ds.GetTarget(resource, FZ_LAST_SEEN_TIMESTAMP, true);
    if (currentTimeStamp)
      ds.Change(resource, FZ_LAST_SEEN_TIMESTAMP, currentTimeStamp, newTimeStamp);
    else
      ds.Assert(resource, FZ_LAST_SEEN_TIMESTAMP, newTimeStamp, true);

    if (!ds.HasAssertion(resource, FZ_FEED, rdf.GetResource(this.feed.url), true))
      ds.Assert(resource, FZ_FEED, rdf.GetResource(this.feed.url), true);

    if (ds.hasArcOut(resource, FZ_VALID))
    {
      var currentValue = ds.GetTarget(resource, FZ_VALID, true);
      ds.Change(resource, FZ_VALID, currentValue, RDF_LITERAL_TRUE);
    }
    else
      ds.Assert(resource, FZ_VALID, RDF_LITERAL_TRUE, true);
  },

  markStored: function(resource)
  {
    var ds = getItemsDS(this.feed.server);

    if (!ds.HasAssertion(resource, FZ_FEED, rdf.GetResource(this.feed.url), true))
      ds.Assert(resource, FZ_FEED, rdf.GetResource(this.feed.url), true);

    var currentValue;
    if (ds.hasArcOut(resource, FZ_STORED))
    {
      currentValue = ds.GetTarget(resource, FZ_STORED, true);
      ds.Change(resource, FZ_STORED, currentValue, RDF_LITERAL_TRUE);
    }
    else
      ds.Assert(resource, FZ_STORED, RDF_LITERAL_TRUE, true);
  },

  mimeEncodeSubject: function(aSubject, aCharset)
  {
    // Get the mime header encoder service
    var mimeEncoder = Components.classes["@mozilla.org/messenger/mimeconverter;1"]
                                .getService(Components.interfaces.nsIMimeConverter);

    // This routine sometimes throws exceptions for mis-encoded data so
    // wrap it with a try catch for now..
    var newSubject;
    try
    {
      newSubject = mimeEncoder.encodeMimePartIIStr(
          this.mUnicodeConverter.ConvertFromUnicode(aSubject),
          false, aCharset, 9, 72);
    }
    catch (ex)
    {
      newSubject = aSubject;
    }

    return newSubject;
  },

  writeToFolder: function()
  {
    debug(this.identity + " writing to message folder" + this.feed.name + "\n");

    var server = this.feed.server;
    this.mUnicodeConverter.charset = this.characterSet;

    // If the sender isn't a valid email address, quote it so it looks nicer.
    if (this.author && this.author.indexOf('@') == -1)
      this.author = '<' + this.author + '>';

    // Convert the title to UTF-16 before performing our HTML entity
    // replacement reg expressions.
    var title = this.title;

    // the subject may contain HTML entities.
    // Convert these to their unencoded state. i.e. &amp; becomes '&'
    title = Components.classes["@mozilla.org/feed-unescapehtml;1"]
                      .getService(Components.interfaces.nsIScriptableUnescapeHTML)
                      .unescape(title);

    // Compress white space in the subject to make it look better.  Trim
    // leading/trailing spaces to prevent mbox header folding issue at just
    // the right subject length.
    title = title.replace(/[\t\r\n]+/g, " ").trim();

    this.title = this.mimeEncodeSubject(title, this.characterSet);

    // If the date looks like it's in W3C-DTF format, convert it into
    // an IETF standard date.  Otherwise assume it's in IETF format.
    if (this.mDate.search(/^\d\d\d\d/) != -1)
      this.mDate = new Date(this.mDate).toUTCString();

    // Escape occurrences of "From " at the beginning of lines of
    // content per the mbox standard, since "From " denotes a new
    // message, and add a line break so we know the last line has one.
    this.content = this.content.replace(/([\r\n]+)(>*From )/g, "$1>$2");
    this.content += "\n";

    // The opening line of the message, mandated by standards to start
    // with "From ".  It's useful to construct this separately because
    // we not only need to write it into the message, we also need to
    // use it to calculate the offset of the X-Mozilla-Status lines from
    // the front of the message for the statusOffset property of the
    // DB header object.
    var openingLine = 'From - ' + this.mDate + '\n';

    var source =
      openingLine +
      'X-Mozilla-Status: 0000\n' +
      'X-Mozilla-Status2: 00000000\n' +
      'X-Mozilla-Keys:                                                                                \n' +
      'Date: ' + this.mDate + '\n' +
      'Message-Id: <' + this.messageID + '>\n' +
      'From: ' + this.author + '\n' +
      'MIME-Version: 1.0\n' +
      'Subject: ' + this.title + '\n' +
      'Content-Transfer-Encoding: 8bit\n' +
      'Content-Base: ' + this.mURL + '\n';

    if (this.enclosure && this.enclosure.mFileName)
    {
      var boundaryID = source.length + this.enclosure.mLength;
      source += 'Content-Type: multipart/mixed;\n boundary="' + ENCLOSURE_HEADER_BOUNDARY_PREFIX + boundaryID + '"' + '\n\n' +
                'This is a multi-part message in MIME format.\n' + ENCLOSURE_BOUNDARY_PREFIX + boundaryID + '\n' +
                'Content-Type: text/html; charset=' + this.characterSet + '\n' +
                'Content-Transfer-Encoding: 8bit\n' +
                this.content;
      source += this.enclosure.convertToAttachment(boundaryID);
    }
    else
    {
      source += 'Content-Type: text/html; charset=' + this.characterSet + '\n' +
                '\n' + this.content;

    }

    debug(this.identity + " is " + source.length + " characters long");

    // Get the folder and database storing the feed's messages and headers.
    var folder = this.feed.folder.QueryInterface(Components.interfaces.nsIMsgLocalMailFolder);
    var msgFolder = folder.QueryInterface(Components.interfaces.nsIMsgFolder);
    msgFolder.gettingNewMessages = true;
    // Source is a unicode string, we want to save a char * string in
    // the original charset. So convert back
    folder.addMessage(this.mUnicodeConverter.ConvertFromUnicode(source));
    msgFolder.gettingNewMessages = false;
  }
};


// A feed enclosure is to RSS what an attachment is for e-mail. We make enclosures look
// like attachments in the UI.

function FeedEnclosure(aURL, aContentType, aLength)
{
  this.mURL = aURL;
  this.mContentType = aContentType;
  this.mLength = aLength;

  // generate a fileName from the URL
  if (this.mURL)
  {
    var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                              .getService(Components.interfaces.nsIIOService);
    var enclosureURL, fileName;
    try
    {
      enclosureURL  = ioService.newURI(this.mURL, null, null)
                               .QueryInterface(Components.interfaces.nsIURL);
      fileName = enclosureURL.fileName;
    }
    catch(ex)
    {
      fileName = this.mURL;
    }
    if (fileName)
      this.mFileName = fileName;
  }
}

FeedEnclosure.prototype =
{
  mURL: "",
  mContentType: "",
  mLength: 0,
  mFileName: "",

  // Returns a string that looks like an e-mail attachment which
  // represents the enclosure.
  convertToAttachment: function(aBoundaryID)
  {
    return '\n' +
                  ENCLOSURE_BOUNDARY_PREFIX + aBoundaryID + '\n' +
                  'Content-Type: ' + this.mContentType +
                                 '; name="' + this.mFileName +
                                 (this.mLength ? '"; size=' + this.mLength : '"') + '\n' +
                  'X-Mozilla-External-Attachment-URL: ' + this.mURL + '\n' +
                  'Content-Disposition: attachment; filename="' + this.mFileName + '"\n\n' +
                  'This MIME attachment is stored separately from the message.\n' +
                  ENCLOSURE_BOUNDARY_PREFIX + aBoundaryID + '--' + '\n';

  }
};
