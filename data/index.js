var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

var updateRanges = true
var editMode = false

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
      button.classList.remove('hover:bg-blue-700')
      button.classList.add('bg-blue-100')
    } else {
      button.classList.remove('bg-blue-100')
      button.classList.add('bg-blue-500')
      button.classList.add('hover:bg-blue-700')
    }
}

function toggleEditMode() {
  editMode = !editMode
  document.getElementById('sensorActions').style.display = editMode ? 'block' : 'none'
  document.getElementById('wifiActions').style.display = editMode ? 'block' : 'none'
  const editButton = document.getElementById('editButton')
  if (editMode) {
    editButton.classList.remove('bg-blue-500', 'hover:bg-blue-700')
    editButton.classList.add('bg-red-500', 'hover:bg-red-700')
    editButton.textContent = 'Leave Edit Mode'
  } else {
    editButton.classList.remove('bg-red-500', 'hover:bg-red-700')
    editButton.classList.add('bg-blue-500', 'hover:bg-blue-700')
    editButton.textContent = 'Enter Edit Mode'
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

    switch (key) {
      case 'steering':
        updateSteeringProgress(value)
        break
      case 'accelerator':
        updateAcceleratorProgress(value)
        break
      case 'brake':
        updateBrakeProgress(value)
        break
      case 'transmission':
        updateTransmission(value)
        break
    }
  })

  if (updateRanges) {
    Object.keys(data.ranges).map((key) => {
    var value = data.ranges[key];
    var input = document.getElementById(key);
    if (input) {
      input.value = value;
      if (key === 'steering_scale') {
        document.getElementById('steering_scale_label').innerText = `Scale: ${value.toFixed(2)}`
      }
    }
  })

    document.getElementById('ssid').value = data.wifi.ssid
    document.getElementById('password').value = data.wifi.password
    updateRanges = false
  }

  document.getElementById('error_brakes').innerHTML = data.error_brakes ? "Yes" : "No"
}

function updateSensorRanges() {
  var data = {};
  const keys = ['steering_range', 'steering_scale', 'accelerator_min', 'accelerator_max', 'brake_min', 'brake_max', 'reverse_threshold', 'low_threshold', 'drive_threshold']
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
  websocket.send(JSON.stringify({ action: 'reboot' }))
}

const MAX_STEERING = 780

function updateSteeringProgress(value) {
  const element = document.getElementById('steering')
  const parent = element.parentElement
  const pixelOffset = (MAX_STEERING - value) / MAX_STEERING * parent.clientWidth / 2
  if (value < 0) {
    element.style.right = '50%';
    element.style.left = `${parent.clientWidth - pixelOffset}px`
    console.log(parent.clientWidth, pixelOffset)
  } else {
    element.style.left = '50%';
    element.style.right = `${pixelOffset}px`
  }
}

function updateAcceleratorProgress(value) {
  const element = document.getElementById('accelerator')
  const parent = element.parentElement
  const pixelOffset = value / 100 * parent.clientWidth
  element.style.width = `${pixelOffset}px`
}

function updateBrakeProgress(value) {
  const element = document.getElementById('brakes')
  const parent = element.parentElement
  const pixelOffset = value / 100 * parent.clientWidth
  element.style.width = `${pixelOffset}px`
}

function updateTransmission(value) {
  let transmissionText = 'Unknown'
  switch (value) {
    case 1:
      transmissionText = 'Reverse'
      break
    case 2:
      transmissionText = 'Neutral'
      break
    case 4:
      transmissionText = 'Drive'
      break
    case 8:
      transmissionText = 'Low'
      break
  }
  document.getElementById('transmission').innerText = transmissionText
}

function updateSteeringScale(value) {
  document.getElementById('steering_scale_label').innerText = `Scale: ${parseFloat(value).toFixed(2)}`
}