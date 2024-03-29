const Cc = Components.classes;
const Ci = Components.interfaces;

var timer;
const start_time = (new Date()).getTime();
const expected_time = 1;

var observer = {
  observe: function observeTC(subject, topic, data) {
    if (topic == "timer-callback") {
      timer.cancel();
      timer = null;

      // expected time may not be exact so convert to seconds and round down.
      var result = Math.floor(((new Date()).getTime() - start_time) / 1000);
      do_check_eq(result, expected_time);

      do_test_finished();
    }
  }
};

function run_test() {
  do_test_pending();

  timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  // Start a 5 second timer, than cancel it and start a 1 second timer.
  timer.init(observer, 5000, timer.TYPE_REPEATING_PRECISE);
  timer.cancel();
  timer.init(observer, 1000, timer.TYPE_REPEATING_PRECISE);
}