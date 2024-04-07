var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

var updateRanges = true

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

function onLoad(event) {
  initWebSocket();
}

// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function toggleButton(id, disabled) {
    const button = document.getElementById(id)
    button.disabled = disabled
    if (disabled) {
        button.classList.remove('bg-blue-500')
        button.classList.add('bg-blue-100')
    } else {
        button.classList.remove('bg-blue-100')
        button.classList.add('bg-blue-500')
    }
}

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen  = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');

  const buttons = ['updateSensorRangeButton', 'updateWiFiButton', 'rebootButton']
  buttons.map((button) => {
    toggleButton(button, false)
  })
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
  const buttons = ['updateSensorRangeButton', 'updateWiFiButton', 'rebootButton']
  buttons.map((button) => {
    toggleButton(button, true)
  })
}

function onMessage(event) {
  var data = JSON.parse(event.data);
  console.log(data);
  if (data.type == 'data') {
  updateData(data);
  }
}

function updateData(data) {
  Object.keys(data.sensors).map((key) => {
  var value = data.sensors[key];
  var element = document.getElementById(`raw_${key}`);
  if (element)
  element.innerText = value;
  })

  Object.keys(data.input).map((key) => {
  var value = data.input[key];
  var input = document.getElementById(key);
  if (input)
  input.innerText = value;
  })

  if (updateRanges) {
    Object.keys(data.ranges).map((key) => {
    var value = data.ranges[key];
    var input = document.getElementById(key);
    if (input)
        input.value = value;
    })

    document.getElementById('ssid').value = data.wifi.ssid
    document.getElementById('password').value = data.wifi.password
    updateRanges = false
  }

  document.getElementById('error_brakes').innerHTML = data.error_brakes ? "Yes" : "No"
}

function updateSensorRanges() {
  var data = {};
  const keys = ['steering_range', 'accelerator_min', 'accelerator_max', 'brake_min', 'brake_max', 'reverse_threshold', 'low_threshold', 'drive_threshold']
  keys.map((key) => {
  var input = document.getElementById(key);
  data[key] = parseFloat(input.value);
  })

  websocket.send(JSON.stringify({action: 'setRanges', data: data}))

  setTimeout(() => {
  updateRanges = true
  }, 1000)
}

function updateWiFi() {
  const data = {
  ssid: document.getElementById('ssid').value,
  password: document.getElementById('password').value
  }
  websocket.send(JSON.stringify({action: 'setWiFi', data: data}))

  setTimeout(() => {
    updateRanges = true
    }, 1000)
}

function reboot() {
  websocket.send(JSON.stringify({action: 'reboot'}))
}