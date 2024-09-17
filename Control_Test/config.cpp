#include "config.h"

const char* indexHtml = R"=====(
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Joystick</title>
  <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
  <script src="https://code.createjs.com/1.0.0/createjs.min.js"></script>
  <script src="https://hammerjs.github.io/dist/hammer.min.js"></script>
  <script>
    const maxValue = 80;
    const minValue = -80;
    const sendInterval = 100; // 每500毫秒發送一次請求
    let totalSize ;
    let joyRadius ;
    let joystickRadius ;
    let maxDistance ;
    let lastSendTime = 0;

    function adjustForScreenSize() {
      if (window.innerHeight < 600) {
        // For small screens
        totalSize = 200;
        joyRadius = 40;
      } else {
        // For large screens
        totalSize = 300;
        joyRadius = 50;
      }
      joystickRadius = totalSize / 2;
      maxDistance = joystickRadius - joyRadius;
    }
    
    function init() {
      adjustForScreenSize()

      const stage = new createjs.Stage('joystick');
      const psp = new createjs.Shape();
      let xCenter = joystickRadius;
      let yCenter = joystickRadius;
      
      psp.graphics.beginFill('#333333').drawCircle(xCenter, yCenter, joyRadius);
      psp.alpha = 0.25;
      stage.addChild(psp);
      
      createjs.Ticker.framerate = 60;
      createjs.Ticker.addEventListener('tick', stage);
      stage.update();

      const myElement = $('#joystick')[0];
      const mc = new Hammer(myElement);

      mc.on("panstart", () => {
        xCenter = psp.x;
        yCenter = psp.y;
        psp.alpha = 0.5;
        stage.update();
      });

      mc.on("panmove", (ev) => {
        const { x, y } = calculateCoords(ev.angle, ev.distance, maxDistance);
        psp.x = x;
        psp.y = y;

        const r = clamp((-1 * x) + (-1 * y));
        const l = clamp(x + (-1 * y));

        $('#xVal').text(`R: ${Math.round(r)}`);
        $('#yVal').text(`L: ${Math.round(l)}`);

        sendMotorValues(l, r);
        stage.update();
      });

      mc.on("panend", () => {
        $('#xVal').text('R: 0');
        $('#yVal').text('L: 0');

        const xhttp = new XMLHttpRequest();
        xhttp.open("GET", `/motor?motor=0,0`, true);
        xhttp.send();

        psp.alpha = 0.25;
        createjs.Tween.get(psp).to({ x: xCenter, y: yCenter }, 750, createjs.Ease.elasticOut);
      });
    }

    function calculateCoords(angle, distance, maxDistance) {
      distance = Math.min(distance, maxDistance);
      const rads = (angle * Math.PI) / 180.0;
      return {
        x: distance * Math.cos(rads),
        y: distance * Math.sin(rads)
      };
    }

    function clamp(value) {
      return Math.max(minValue, Math.min(maxValue, value));
    }

    function sendMotorValues(l, r) {
      var currentTime = new Date().getTime();

      if (currentTime - lastSendTime >= sendInterval) {
        
        lastSendTime = currentTime;

        const xhttp = new XMLHttpRequest();
        xhttp.open("GET", `/motor?motor=${l},${r}`, true);
        xhttp.send();
      }
    }

    function led(value) {
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", `/led?duty=${value}`, true);
      xhttp.send();
    }

    function servo(value) {
      var xhttp = new XMLHttpRequest();
      xhttp.open("GET", `/servo?duty=${value}`, true);
      xhttp.send();
    }
  </script>
  <style>
    body {
      margin: 0;
      padding: 0;
      overflow: hidden;
      touch-action: none;
    }

    .noselect {
      user-select: none;
      -webkit-user-select: none;
      -moz-user-select: none;
      -ms-user-select: none;
    }

    #joystick {
      height: 300px;
      width: 300px;
      border-radius: 50%;
      background-color: #DEB887;
      position: absolute;
      bottom: 0;
      left: 0;
      margin: 20px;
    }

    #camImage {
      background-color: #DEB887;
      position: absolute;
      bottom: 0;
      left: 50%;
      margin: 20px;
    }

    header {
      background-color: #8B5A2B;
      color: #fff;
      padding: 20px;
    }

    input[type="range"] {
      -webkit-appearance: none;
      appearance: none;
      width: 150px;
      height: 10px;
      background: #ddd;
      border-radius: 5px;
    }

    input[type="range"]::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: #DEB887;
      cursor: pointer;
    }

    .container {
      display: flex;
      justify-content: flex-start;
      padding: 0;
      list-style-type: none;
    }

    .container div {
      margin: 0 10px;
    }

    .container div:last-child {
      margin-left: auto;
    }
    
    @media (max-height: 600px) {
      #joystick {
        height: 200px;
        width: 200px;
      }
    }
  </style>
</head>
<body class="noselect" onload="init();">
  <header>
    <div class="container noselect">
      <div id="yVal">L : 0</div>
      <div id="xVal">R : 0</div>
      <div>
        <nobr>LED Light : </nobr>
        <input type="range" id="slider" min="0" max="255" value="0" step="10" onchange="led(this.value)">
        <nobr>Servo : </nobr>
        <input type="range" id="slider" min="0" max="100" value="100" step="10" onchange="servo(this.value)">
      </div>
    </div>
  </header>
  <div>
    <canvas id="joystick" height="300" width="300"></canvas>
    <!--<img id="camImage" src="imgtest.png" alt="test">-->
  </div>
</body>
</html>
)=====";
