// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

(async function() {
  TestRunner.addResult(`Tests that resources panel shows form data parameters.\n`);
  await TestRunner.loadModule('network_test_runner');
  await TestRunner.navigatePromise('http://[::1]:8000/devtools/resources/inspected-page.html');
  await TestRunner.evaluateInPagePromise(`
      document.write(\`<form target="target-iframe" method="POST" action="http://[::1]:8000/devtools/resources/post-target.cgi?queryParam1=queryValue1&amp;queryParam2=#fragmentParam1=fragmentValue1&amp;fragmentParam2=">
      <input name="formParam1" value="formValue1">
      <input name="formParam2">
      <input id="submit" type="submit">
      </form>
      <iframe name="target-iframe"></iframe>
      <script>
      function submit()
      {
          document.getElementById("submit").click();
      }
      </script>
    \`)`);

  TestRunner.evaluateInPage('submit()');
  TestRunner.networkManager.addEventListener(SDK.NetworkManager.Events.RequestFinished, onRequestFinished);

  async function onRequestFinished(event) {
    var request = event.data;
    TestRunner.addResult(request.url());
    TestRunner.addObject(await NetworkLog.HAREntry.build(request), NetworkTestRunner.HARPropertyFormattersWithSize);
    TestRunner.completeTest();
  }
})();
