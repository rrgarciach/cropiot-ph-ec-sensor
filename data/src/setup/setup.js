$(function () {

  const API = {
    WIFI: {
      STATUS: '/wifi/status',
      CONNECT: '/wifi/connect',
      SCAN: '/wifi/scan',
    },
  };

  const ssidDropdown = {
    menu: $('#ssid div.dropdown-menu'),
    items: $('#ssid a.dropdown-item'),
    button: $('#ssid button'),
    input: $('#ssid input'),
  };
  const inputPass = $('input#pass');
  const wifiSubmitButton = $('#wifi-submit');

  scanNetworks();

  function getStatus() {
    return $.get(API.WIFI.STATUS)
    .then((data = {}) => {
      ssidDropdown.input.val(data.ssid);
      inputPass.val(data.pass);
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
    });
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

  ssidDropdown.items.on('click', function () {
    const value = $(this).text();
    ssidDropdown.button.text(value);
    ssidDropdown.input.val(value);
  });

});
