/*
 * Copyright (c) 2018, Henry Corrigan-Gibbs
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

ChromeUtils.import('resource://gre/modules/Services.jsm');
Services.prefs.setStringPref('prio.publicKeyA', arguments[0]);
Services.prefs.setStringPref('prio.publicKeyB', arguments[1]);

var prio = new PrioEncoder();

var l = prio.encode(arguments[2], {
  'hasCrashedInLast24Hours': Number(arguments[3]),
  'hasDefaultHomepage': Number(arguments[4]),
  'hasUsedPrivateBrowsing': Number(arguments[5])
}).then(function(v) {
  function intToHex(x) { return x.toString(16).padStart(2, '0'); };
  function toHexString(arr) { return arr.map(intToHex).join(','); };
  console.log(toHexString(v[0]) + ',$' + toHexString(v[1]));
  console.log('');
}, function(v) {
  console.log('Failure.');
  console.log(v);
});
