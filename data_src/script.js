  var gateway = `ws://${window.location.hostname}/ws`;
  var websocket;
  // Init web socket when the page loads
  window.addEventListener('load', onLoad);


 function onLoad(event) {
    initWebSocket();
    getButton();
    setIRstate();
    setb203();
    getb203();
    setEventb203();
    setDateb203();
    setTimeb203();
    callEventb203();
    delEventb203();
    testEventb203();
    getb215();
    getb226();
  }

    function initWebSocket() {
    console.log('Trying to open a WebSocket connection...');
    websocket = new WebSocket(gateway);
    websocket.onopen    = onOpen;
    websocket.onclose   = onClose;
    websocket.onmessage = onMessage; // <-- add this line
  }
  
// When websocket is established, call the getReadings() function
  function onOpen(event) {
    console.log('Connection opened');
    websocket.send('ConnectionOpened');
  }

  function onClose(event) {
    console.log('Connection closed');
    setTimeout(initWebSocket, 2000);
  }

  function onMessage(event) {
    console.log(event.data);
    const Identifier = event.data.slice(1, 3);
    let NoId = 0;
    let PR99 = 1;
    let A725 = 2;
    let B285 = 3;
    let B215 = 4;
    let B225_2 = 5;
    let B226 = 6;
    let A725_2 = 7;
    let B291 = 8;
    let B203 = 50;
    if (Number(Identifier) == NoId) {
        //Nothing to do
      } else if  (Number(Identifier) == PR99 ){

      } else if  (Number(Identifier) == A725 ){

      } else if  (Number(Identifier) == B285 ){

      } else if  (Number(Identifier) == B215 ){
        getB215Settings();
      } else if  (Number(Identifier) == B225_2 ){

      } else if  (Number(Identifier) == B226 ){
        getB226Settings();
      } else if  (Number(Identifier) == A725_2 ){

      } else if  (Number(Identifier) == B291 ){

      } else if  (Number(Identifier) >= B203) {
        getB203Settings();
      }
  }

function setIRstate() {

  document.getElementById('IR')?.addEventListener('change', () => {
          const Id = event.target.id;
          const Value = event.target.checked;
          const Name = event.target.name;
          const Class = event.target.className;
          if (websocket.readyState === WebSocket.OPEN) {
          console.log(Name + Value + Id);
          websocket.send(Name + Value + Id);
         }
      });
}

function setb203() {

  document.getElementById('b203_set')?.addEventListener('click', () => {
    const Name = event.target.name;
    const language = document.querySelector('select[id="language"]').value;
    const easy = document.querySelector('select[id="easy"]').value;
    const timer = document.querySelector('select[id="timer"]').value;
    const poweron = document.querySelector('select[id="poweron"]').value;
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( Name + "0S" + language + easy + timer + poweron );
    websocket.send( Name + "0S" + language + easy + timer + poweron );
    }
  });
}

function getb215() {

  document.getElementById('b215_get')?.addEventListener('click', () => {
    const Name = event.target.name;
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( "tape2X" );
    websocket.send("tape2X");
    }
  });
}

function getb226() {

  document.getElementById('b226_get')?.addEventListener('click', () => {
    const Name = event.target.name;
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( "cdplayerX" );
    websocket.send("cdplayerX");
    }
  });
}

function getb203() {

  document.getElementById('b203_get')?.addEventListener('click', () => {
    const Name = event.target.name;
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( Name + "0X" );
    websocket.send( Name + "0X");
    }
  });
}

function setDateb203() {

 document.getElementById('setDate')?.addEventListener('click', () => {
        const Name = event.target.name;
        const setdate = document.querySelector("#set_date").value;
        let millenShort  = setdate.slice(2)
        let splitdate = millenShort.replace(/-/g, "");
        const swaped = swapPairs(splitdate);
        if (websocket.readyState === WebSocket.OPEN) {
        console.log( Name + "0D" + swaped );
        websocket.send( Name + "0D" + swaped );
        }
    });
}

function setTimeb203() {

  document.getElementById('setTime')?.addEventListener('click', () => {
    const Name = event.target.name;
    const setdate = document.querySelector("#set_time").value;
    let formtime = setdate.replace(/:/g, "");
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( Name + "0T" + formtime );
    websocket.send( Name + "0T" + formtime );
    }
  });
}

function callEventb203() {

  document.getElementById('b203callevent')?.addEventListener('click', () => {
    const Name = event.target.name;
    const b203call = document.querySelector('select[id="b203evn"]').value;
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( Name + "0C" + b203call );
    websocket.send( Name + "0C" + b203call );
    }
  });
}

function delEventb203() {

  document.getElementById('b203delevent')?.addEventListener('click', () => {
    const Name = event.target.name;
    const b203call = document.querySelector('select[id="b203evn"]').value;
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( Name + "0E" + b203call + "E" );
    websocket.send( Name + "0E" + b203call + "E" );
    }
  });
}

function testEventb203() {

  document.getElementById('b203testevent')?.addEventListener('click', () => {
    const Name = event.target.name;
    const b203test = document.querySelector('select[id="b203evn"]').value;
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( Name + "0V" + b203test  );
    websocket.send( Name + "0V" + b203test );
    }
  });
}

function setEventb203() {

  document.getElementById('b203setevent')?.addEventListener('click', () => {
    const Name = event.target.name;
    const b203evn = document.querySelector('select[id="b203evn"]').value;
    const datatype = document.querySelector('select[id="datetype"]').value;
    const weekday  = document.querySelectorAll('input[name="day"]');
    let weekdayString = "";
    weekday.forEach((checkbox) => {
    weekdayString += checkbox.checked ? "1" : "0";
    });
    const signalsource = document.querySelector('select[id="signalsource"]').value;
    const sourceadd = document.querySelector("#sourceadd").value;
    const output  = document.querySelectorAll('input[name="out"]');
    let outputString = "";
    output.forEach((checkbox) => {
    outputString += checkbox.checked ? "1" : "0";
    });
    const eventstarttime = document.querySelector("#eventstarttime").value;
    let formevstarttime = eventstarttime.replace(/:/g, "");
    const eventsoptime = document.querySelector("#eventstoptime").value;
    let formevstoptime = eventsoptime.replace(/:/g, "");
    if (websocket.readyState === WebSocket.OPEN) {
    console.log( Name + "0E" + b203evn + datatype + weekdayString + signalsource + sourceadd + outputString + formevstarttime + formevstoptime);
    websocket.send( Name + "0E" + b203evn + datatype + weekdayString + signalsource + sourceadd + outputString + formevstarttime + formevstoptime );
    }
  });
}

function getButton() {

const buttons = document.querySelectorAll('.button, .misc_button, .power_btn');

    buttons.forEach(btn => {
      btn.addEventListener('mousedown', (event) => {
        const Id = event.target.id;
        const Name = event.target.name;
        const Class = event.target.className;
        const isObjectEmpty = (Name) => {
        return Object.keys(objectName).length === 0
        }
        if (websocket.readyState === WebSocket.OPEN) {
        console.log(Name + 'Push' + Id );
        websocket.send(Name + 'Push' + Id);
        }
      })
    });

    buttons.forEach(btn => {
      btn.addEventListener('mouseup', (event) => {
        const Id = event.target.id;
        const Name = event.target.name;
        const Class = event.target.className;
        const isObjectEmpty = (Name) => {
        return Object.keys(objectName).length === 0
        }
        if (websocket.readyState === WebSocket.OPEN) {
        console.log(Name + 'Release' + Id );
        websocket.send(Name + 'Release' + Id);
        }
      })
    });
  }

function openB203(evt, TabName) {
  // Declare all variables
  var i, tabcontent, tablinks;

  // Get all elements with class="tabcontent" and hide them
  tabcontent = document.getElementsByClassName("tabcontent");
  for (i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // Get all elements with class="tablinks" and remove the class "active"
  tablinks = document.getElementsByClassName("tablinks");
  for (i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // Show the current tab, and add an "active" class to the button that opened the tab
  document.getElementById(TabName).style.display = "block";
  evt.currentTarget.className += " active";
} 

function swapPairs(str) {
  if (str.length < 6) return "String zu kurz";
  let p1 = str.substring(0, 2);
  let p2 = str.substring(2, 4);
  let p3 = str.substring(4, 6);
  return p3 + p2 + p1;
}

function getB203Settings() {
   const datetimestr = event.data.slice(3, 15);
        // 1. String zerlegen mittels substring
        const day = datetimestr.substring(0, 2);
        const month = datetimestr.substring(2, 4);
        const year = datetimestr.substring(4, 6);
        const yearlong = ("20" + year);
        const hour = datetimestr.substring(6, 8);
        const minute = datetimestr.substring(8, 10);
        const second = datetimestr.substring(10, 12);
        // 2. Formatieren für HTML-Inputs
        const dateValue = `${yearlong}-${month}-${day}`; // YYYY-MM-DD
        const timeValue = `${hour}:${minute}:${second}`;       // HH:mm
        // 3. Werte in die Elemente einsetzen
        document.getElementById("set_date").value = dateValue;
        document.getElementById("set_time").value = timeValue;
        const rawdata = event.data.slice(-6);
        const lang = rawdata.charAt(0);
        document.getElementById("language").value = lang;
        const easymod = rawdata.charAt(1);
        document.getElementById("easy").value = easymod;
        const time = rawdata.charAt(2);
        document.getElementById("timer").value = time;
        const pwron = rawdata.charAt(3);
        document.getElementById("poweron").value = pwron;
        const irled = rawdata.charAt(4);
          if (irled == "0") {
          document.getElementById("IR").checked = true;
          } else if (irled == "1") {
          document.getElementById("IR").checked = false;
          }
}

function getB215Settings() {
        const rawdata = event.data.slice(3);
        const functions = rawdata.charAt(0);
        document.getElementById("functions").value = functions;
        const addfunctions = rawdata.charAt(1);
        document.getElementById("addfunctions").value = addfunctions;
        const cuestate = rawdata.charAt(2);
        document.getElementById("cuestate").value = cuestate;
        const tapecountermm = rawdata.slice(3, 5);
        const tapecounterss = rawdata.slice(5, 7);
        const tapecounter = (tapecountermm + ":" + tapecounterss)
        document.getElementById("bandzaehler").value = tapecounter;
}

function getB226Settings() {
        const rawdata = event.data.slice(3);
        const b226state = rawdata.charAt(0);
        document.getElementById("b226state").value = b226state;
        const tracknumber = rawdata.slice(1, 3);
        document.getElementById("tracknumber").value = tracknumber;
        const indexnumber = rawdata.slice(3, 5);
        document.getElementById("indexnumber").value = indexnumber;
        const elapsedtimemm = rawdata.slice(5, 7);
        const elapsedtimess = rawdata.slice(7, 9);
        const elapsedtime = (elapsedtimemm + ":" + elapsedtimess);
        document.getElementById("elapsedtime").value = elapsedtime;
        const remainingtimemm = rawdata.slice(9, 11);
        const remainingtimess = rawdata.slice(11, 13);
        const remainingtime = (remainingtimemm + ":" + remainingtimess);
        document.getElementById("remainingtime").value = remainingtime;
}