const char html[] PROGMEM = R"rawliteral( 
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset=utf-8>
  <title>
  </title>
  <style>
    canvas {
      padding-left: 0;
      padding-right: 0;
      margin-left: auto;
      margin-right: auto;
      display: block;
      width: 800px;
      height: 800px;
      border: 2px solid gray;
    }
    button {
      font-size: 2rem;
      border-radius: 1rem;
      padding: 1rem;
    }
  </style>
  <script src="https://ajax.googleapis.com/ajax/libs/jquery/1.7.2/jquery.min.js"></script>
  <script src="https://ajax.googleapis.com/ajax/libs/jqueryui/1.8.18/jquery-ui.min.js"></script>
  <script>
    const ARM_WIDTH = 30;
    const ARM_LENGTH = 550;
    
    const CANVAS_WIDTH =  800
    const CANVAS_HEIGHT = 800

    function moveMotors() {
      debugger;

      const motor1 = $("#motor1").val();
      const motor2 = $("#motor2").val();
      const motor3 = $("#motor3").val();

      fetch ("/move?motor1=" + motor1 + "&motor2=" + motor2 +"&motor3=" + motor3)
        .then(response => response.json())
        .then(jsonData => console.log(jsonData));
    }
  </script>
  <canvas id="main" width="800" height="800"></canvas>

  <div>
    <hr />
    <button id="clear">clear</button>
   <button id="reset">Reset</button>
   <button id="track-corners">Track Corners</button>

    <ul>
      <li>Motor 1: <input id="motor1" value="-1" type="number"></input></li>
      <li>Motor 2: <input id="motor2" value="-1" type="number"></input></li>
      <li>Motor 3: <input id="motor3" value="-1" type="number"></input></li>
    </ul>
    <button onclick="moveMotors()">Move Motors</button>
    Dot:
    <ul>
      <li>x: <input id="dot-x" value="-1" type="number"></input></li>
      <li>y: <input id="dot-y" value="-1" type="number"></input></li>
    </ul>
     <button onclick="drawDot({x:$('dot-x').val(),y:$('dot-y').val()})">Draw Dot</button>
  </div>
  <script>
      const MOTOR3_DRAWING_ANGLE = 90;
      const MOTOR3_NOT_DRAWING_ANGLE = 0;
      const radiansToDegrees = rad => 180 / Math.PI * rad;

      const requestMotorMove = pos => {


        // Ensure the arms can reach the far corner of the canvas
        const effectiveArm = Math.max(ARM_LENGTH  - ARM_WIDTH / 2, Math.sqrt(Math.pow(CANVAS_WIDTH,2) + Math.pow(CANVAS_HEIGHT,2)) / 2); 

        const effectiveX = CANVAS_WIDTH - pos.x;
        const effectiveY = CANVAS_HEIGHT - pos.y;
        const arm1Degree = 180 - radiansToDegrees(Math.atan(effectiveY/effectiveX) + Math.acos(Math.sqrt(Math.pow(effectiveX,2)+Math.pow(effectiveY,2))/(2*effectiveArm)));
        const arm2Degree = 180 - radiansToDegrees(Math.acos(1 - (Math.pow(effectiveX, 2) + Math.pow(effectiveY, 2))/(2*effectiveArm*effectiveArm))); 
        const palmDegree = pos.isDrawing == null ? -1 : (pos.isDrawing === true ? MOTOR3_DRAWING_ANGLE : MOTOR3_NOT_DRAWING_ANGLE);

        console.log (effectiveX, effectiveY, arm1Degree.toFixed(2), arm2Degree.toFixed(2));

        return fetch ("/move?motor1=" + arm1Degree.toFixed(2) + "&motor2=" + arm2Degree.toFixed(2) +"&motor3=" + palmDegree)
          .then(response => response.text())
          .then(resopnse => console.log(resopnse));
      };

      function debounce(cb, delay = 250) {
        let timeout

        return (...args) => {
          clearTimeout(timeout)
          timeout = setTimeout(() => {
            cb(...args)
          }, delay)
        }
      }

      const resetLocation = () => fetch ("/move?motor1=0&motor2=180&motor3-1");

      const trackCorners = () => {
        return requestMotorMove({x: 0, y:0})
          .then(_ => requestMotorMove({x: CANVAS_WIDTH / 2, y:0}))
          .then(_ => requestMotorMove({x: CANVAS_WIDTH, y:0}))
          .then(_ => requestMotorMove({x: CANVAS_WIDTH - 5, y:(CANVAS_HEIGHT - 5)/2}))
          .then(_ => requestMotorMove({x: CANVAS_WIDTH - 5, y:CANVAS_HEIGHT - 5}))
          .then(_ => requestMotorMove({x: 0, y:CANVAS_HEIGHT}))
          .then(_ => requestMotorMove({x: 0, y:CANVAS_HEIGHT /2}))
          .then(_ => requestMotorMove({x: 0, y:0}))
      };

      const drawDot = pos => {
         return requestMotorMove({...pos, isDrawing: false})
            .then(_ => requestMotorMove({...pos, isDrawing: true}))
            .then(_ => requestMotorMove({...pos, isDrawing: false}));
      }

      window.onload = function() {

        document.ontouchmove = function(e){ e.preventDefault(); }

        const canvas  = document.getElementById('main');
        const canvastop = canvas.offsetTop

        const context = canvas.getContext("2d");

        let isDrawing = false;
        const lastPos = {x:0, y:0};

        context.strokeStyle = "#000000";
        context.lineCap = 'round';
        context.lineJoin = 'round';
        context.lineWidth = 5;

        function clear() {
          context.fillStyle = "#ffffff";
          context.rect(0, 0, 800, 800);
          context.fill();
        }

        function dot(pos) {
          context.beginPath();
          context.fillStyle = "#000000";
          context.arc(pos.x,pos.y,1,0,Math.PI*2,true);
          context.fill();
          context.stroke();
          context.closePath();
        }

        function line(from, to) {
          context.beginPath();
          context.moveTo(from.x, from.y);
          context.lineTo(to.x, to.y);
          context.stroke();
          context.closePath();

          if (from.x !== to.x && from.y !== to.y){
            debounce(()=>{
              requestMotorMove(to);
            }, 150)();
          }
        }

        function draw(to){
          line(lastPos, to);
          
          lastPos.x = to.x;
          lastPos.y = to.y;
        }

        function getMousePos(canvasDom, mouseEvent) {
          const rect = canvasDom.getBoundingClientRect();
          return {
            x: mouseEvent.clientX - rect.left,
            y: mouseEvent.clientY - rect.top
          };
        }

        canvas.ontouchstart = function(event){                   
          event.preventDefault();                 
          
          lastPos.x = event.touches[0].clientX;
          lastPos.y = event.touches[0].clientY - canvastop;

          dot(lastPos);
        }

        canvas.ontouchmove = function(event){                   
          event.preventDefault();                 

          const pos = {
            x: event.touches[0].clientX,
            y: event.touches[0].clientY - canvastop          
          };

          draw(pos);
        }

        canvas.addEventListener("mousedown", function (e) {
          isDrawing = true;
          const pos = getMousePos(canvas, e);
          dot(pos);
          fetch ("/move?motor1=-1&motor2=-1&motor3=" + MOTOR3_DRAWING_ANGLE);

          lastPos.x = pos.x;
          lastPos.y = pos.y;
        }, false);

        canvas.addEventListener("mouseup", function (e) {
          isDrawing = false;
          fetch ("/move?motor1=-1&motor2=-1&motor3=" + MOTOR3_NOT_DRAWING_ANGLE);
        }, false);

        canvas.addEventListener("mousemove", function (e) {
          const pos = getMousePos(canvas, e);
          if (isDrawing){
            draw(pos);
          }
        }, false);

        const clearButton = document.getElementById('clear')
        clearButton.onclick = clear

        const resetButton = document.getElementById('reset');
        resetButton.onclick = resetLocation;

        const trackCornersButton = document.getElementById('track-corners');
        trackCornersButton.onclick = trackCorners;

        clear()
      }
  </script>
<body>

</body>
</html>)rawliteral";