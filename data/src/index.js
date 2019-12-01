;$(function () {

  const API = {
    WIFI: {
      STATUS: '/api/settings/wifi/status',
      CONNECT: '/api/settings/wifi/connect',
      SCAN: '/api/settings/wifi/scan',
    },
  };


  const WL_NO_SHIELD        = 255;
  const WL_IDLE_STATUS      = 0;
  const WL_NO_SSID_AVAIL    = 1;
  const WL_SCAN_COMPLETED   = 2;
  const WL_CONNECTED        = 3;
  const WL_CONNECT_FAILED   = 4;
  const WL_CONNECTION_LOST  = 5;
  const WL_DISCONNECTED     = 6;

  const ssidDropdown = {
    menu: $('#ssid div.dropdown-menu'),
    items: '#ssid a.dropdown-item',
    button: $('#ssid button'),
    input: $('#ssid input'),
  };
  const inputPass = $('input#pass');
  const wifiSubmitButton = $('#wifi-submit');

  function redirectToSetup() {
    window.location.replace('./src/setup/setup.html');
  }

  function getStatus() {
    return $.get(API.WIFI.STATUS)
    .then((data = {}) => {
      if (!data.ssid) return Promise.resolve(WL_DISCONNECTED);
      ssidDropdown.input.val(data.ssid);
      inputPass.val(data.pass);
      return Promise.resolve(WL_CONNECTED);
    });
  }

  function connect(data) {
    return $.post(API.WIFI.CONNECT, data)
    .then(() => getStatus())
    .fail(function (err) {
      alert('Failed to connect with selected network');
    });
  }

  function scanNetworks() {
    return $.get(API.WIFI.SCAN)
    .then((networks = []) => {
      ssidDropdown.menu.text('');
      networks.map(n => {
        const el = `<a class="dropdown-item" href="#">${n.ssid}</a>`;
        ssidDropdown.menu.append(el);
      });
    })
    .fail(function (err) {
      const el = '<a class="dropdown-item disabled" href="#">No networks were found.</a>';
      ssidDropdown.menu.text('');
      ssidDropdown.menu.append(el);
    })
    .always(() => {
      $('#ssid a.dropdown-item').on('click', function () {
        const value = $(this).text();
        console.log(value)
        ssidDropdown.button.text(value);
        ssidDropdown.input.val(value);
      });
    })
  }

  wifiSubmitButton.click(function (e) {
    e.preventDefault();
    const data = {
      ssid: ssidDropdown.input.val(),
      pass: inputPass.val(),
    };
    if (!data.ssid) alert('Invalid SSID');
    else if (!data.pass) alert('Invalid password');
    else connect(data);
  });

  $('span.input-group-text').on('click', function () {
    if (inputPass.attr('type') === 'password') {
      $(this).text('hide');
      inputPass.attr('type', 'text');
    } else {
      $(this).text('show');
      inputPass.attr('type', 'password');
    }
  });

  getStatus()
  .then(status => {
    console.log(status, WL_CONNECTED)
    if (status === WL_CONNECTED) {
      redirectToSetup();
    } else {
      scanNetworks();
    }
  });

});
