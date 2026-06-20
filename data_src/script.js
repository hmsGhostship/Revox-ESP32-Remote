const gateway = `ws://${window.location.hostname}/ws`;
let websocket;

let portconfigData = [];
 // Globale Variable, um alle restlichen Daten im Hintergrund zu behalten
let configData = [];


window.addEventListener('load', onLoad);
//window.addEventListener('DOMContentLoaded', loadConfig);

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
    getb291();
    getb285();
    setB285Speakers();
    setB285Volume();
    setPorts();
  }


// 1. FEHLER BEHOBEN: Anführungszeichen bei "'" korrekt maskiert
const escapeHtml = (str) => String(str).replace(/[&<>"']/g, m => ({
  '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;'
})[m]);

// ERGÄNZT: Diese Funktion wird von den Select-Feldern in der Tabelle benötigt
function updateValue(index, key, value) {
  if (portconfigData[index]) {
    portconfigData[index][key] = value;
    console.log("Wert im Array aktualisiert:", portconfigData);
  }
}

// Zeichnet die Tabelle basierend auf dem aktuellen Zustand von portconfigData neu
function renderTable() {
  const tbody = document.getElementById('table-body');
  if (!tbody) return;

  const rows = portconfigData.map((item, index) => {
    const safeName = escapeHtml(item.name || '');
    const safeDescr = escapeHtml(item.descr || '');
    
    return `
      <tr>
        <td><input class="portselect" type="text" value="${safeName}" autocomplete="off" disabled></td>
        <td><input class="portselect" type="text" value="${safeDescr}" autocomplete="off" disabled></td>
        <td>
          <select class="portselect" onchange="updateValue(${index}, 'out', this.value)">
            <option value="no" ${item.out === 'no' ? 'selected' : ''}>Nicht verf&uuml;gbar</option>
            ${[...Array(10).keys()].map(i => `
              <option value="${i}" ${item.out == i ? 'selected' : ''}>${i}</option>
            `).join('')}
          </select>
        </td>
        <td><input type="checkbox" ${item.feedback ? 'checked' : ''} disabled></td>
      </tr>
    `;
  });

  tbody.innerHTML = rows.join('');
}

function syncSelectField() {
  const selectElem = document.getElementById('change');
  if (!selectElem) return;

  // Wir prüfen primär, ob der Alternativzustand B251 aktiv ist
  const isB251Active = portconfigData.some(item => item.name === "B251");

  // Optionen generieren und den Zustand exakt setzen
  selectElem.innerHTML = `
    <option value="B285" ${!isB251Active ? 'selected' : ''}>B285 (Standard)</option>
    <option value="B251" ${isB251Active ? 'selected' : ''}>B251 (+ Tuner B261)</option>
  `;
}

function switchConfiguration(selectedValue) {
  if (selectedValue === "B251") {
    // === ZUSTAND B251 AKTIVIEREN ===
    
    // 1. "B285" suchen und alle drei Werte anpassen
    const entry = portconfigData.find(item => item.name === "B285");
    if (entry) {
      entry.name = "B251";
      entry.descr = "amplifier"; // Von "receiver" zu "amplifier"
      entry.out = "1";
      entry.feedback = false;    // Entspricht der Checkbox (0 / unchecked)
    }

    // 2. Den Tuner "B261" hinzufügen, falls noch nicht da
    const exists261 = portconfigData.some(item => item.name === "B261");
    if (!exists261) {
      portconfigData.push({
        "name": "B261",
        "descr": "tuner",
        "out": "4",
        "feedback": false
      });
    }

  } else if (selectedValue === "B285") {
    // === ZUSTAND B285 AKTIVIEREN (ZURÜCKSETZEN) ===
    
    // 1. "B251" suchen und wieder auf die alten Werte setzen
    const entry = portconfigData.find(item => item.name === "B251");
    if (entry) {
      entry.name = "B285";
      entry.descr = "receiver";  // Zurück zu "receiver"
      entry.out = "1";
      entry.feedback = true;     // Zurück zu 1 (checked)
    }

    // 2. Den Tuner "B261" wieder komplett entfernen
    portconfigData = portconfigData.filter(item => item.name !== "B261");
  }

  // UI komplett aktualisieren (Tabelle spiegelt alle Änderungen sofort wider)
  renderTable();
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
    websocket.send('get_data');
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
        getB285Settings();
      } else if  (Number(Identifier) == B215 ){
        getB215Settings();
      } else if  (Number(Identifier) == B225_2 ){

      } else if  (Number(Identifier) == B226 ){
        getB226Settings();
      } else if  (Number(Identifier) == A725_2 ){

      } else if  (Number(Identifier) == B291 ){
        getB291Settings();
      } else if  (Number(Identifier) >= B203) {
        getB203Settings();
      }
  }

  function setIRstate() {

    document.getElementById('IR')?.addEventListener('change', () => {
          const Id = event.target.id;
          const Value = event.target.checked;
          const Name = event.target.name;
          if (websocket.readyState === WebSocket.OPEN) {
          console.log(Name + Value + Id);
          websocket.send(Name + Value + Id);
         }
    });
  }

  function setB285Speakers() {

    document.getElementById('speakers')?.addEventListener('change', () => {
          const Id = event.target.id;
          const Value = event.target.value;
          const Name = event.target.name;
          if (websocket.readyState === WebSocket.OPEN) {
          console.log(Name + Value);
          websocket.send(Name + Value);
         }
    });
  }

  function setB285Volume() {

    document.getElementById('volumeSlider')?.addEventListener('change', () => {
          const Id = event.target.id;
          const Value = event.target.value;
          const Name = event.target.name;
          if (websocket.readyState === WebSocket.OPEN) {
          console.log(Name + "V" + Value);
          websocket.send(Name + "V" +Value);
         }
    });
  }

  function setPorts() {

    document.getElementById('b203setport')?.addEventListener('click', () => {
    // 1. Alle Elemente mit der gewünschten Klasse auswählen (z.B. "meine-klasse")
    const elemente = document.querySelectorAll('.portConfig');
    // 2. Die Werte der Elemente in ein Array oder Objekt extrahieren
      const datenObjekte = Array.from(elemente).map((element) => {
        return {
          descr: element.getAttribute('data-descr'), // Beispiel für ein weiteres Attribut (data-*)
          out: element.value, // Der Hauptwert/Text des Elements
          feedback: element.getAttribute('data-feedback'), // Beispiel für ein weiteres Attribut (data-*)
        };
      });
      // 3. Das Array/Objekt in einen JSON-String umwandeln
      const jsonErgebnis = JSON.stringify(datenObjekte, null, 2);
      console.log(jsonErgebnis);
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

  function getb285() {

    document.getElementById('b285_get')?.addEventListener('click', () => {
      const Name = event.target.name;
      if (websocket.readyState === WebSocket.OPEN) {
        console.log( "receiverX" );
        websocket.send("receiverX");
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

  function getb291() {

    document.getElementById('b291_get')?.addEventListener('click', () => {
      const Name = event.target.name;
      if (websocket.readyState === WebSocket.OPEN) {
        console.log( "phonoX" );
        websocket.send( "phonoX");
      }
    });
  }

  function getb291tabevent() {
  if (websocket.readyState === WebSocket.OPEN) {
      console.log( "phonoX" );
      websocket.send("phonoX");
    }
  }

  function getb226tabevent() {
    if (websocket.readyState === WebSocket.OPEN) {
      console.log( "cdplayerX" );
      websocket.send("cdplayerX");
    }
  }

  function getb215tabevent() {
    if (websocket.readyState === WebSocket.OPEN) {
      console.log( "tape2X" );
      websocket.send("tape2X");
    }
  }

  function getb203tabevent() {
      if (websocket.readyState === WebSocket.OPEN) {
        console.log( "getsettings0X" );
        websocket.send( "getsettings0X");
      }
  }

  function getWIFIConfig() {
      fetch('/get-config')
        .then(response => response.json())
        .then(data => {
          document.getElementById('ssid').value = data.ssid;
          document.getElementById('password').value = data.password;
        });
  }



  function getb285tabevent() {
    if (websocket.readyState === WebSocket.OPEN) {
      console.log( "receiverX" );
      websocket.send("receiverX");
    }
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
      const b203del = document.querySelector('select[id="b203evn"]').value;
      if (websocket.readyState === WebSocket.OPEN) {
      console.log( Name + "0E" + b203del + "E" );
      websocket.send( Name + "0E" + b203del + "E" );
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
    
    // --- 1. BLOCK: DRÜCKEN (mousedown / touchstart) ---
    ['mousedown', 'touchstart'].forEach(eventType => {
      btn.addEventListener(eventType, (event) => {
        if (event.cancelable) event.preventDefault(); // Stoppt iOS-Doppelklicks

        const Id = event.currentTarget.id;
        const Name = event.currentTarget.name;

        // WebSocket: Push senden
        if (websocket.readyState === WebSocket.OPEN) {
          console.log(Name + 'Push' + Id);
          websocket.send(Name + 'Push' + Id);
        }
      }, { passive: false });
    });

    // --- 2. BLOCK: LOSLASSEN (mouseup / touchend) ---
    ['mouseup', 'touchend'].forEach(eventType => {
      btn.addEventListener(eventType, (event) => {
        if (event.cancelable) event.preventDefault();

        const Id = event.currentTarget.id;
        const Name = event.currentTarget.name;

          // WebSocket: Release senden
          if (websocket.readyState === WebSocket.OPEN) {
            console.log(Name + 'Release' + Id);
            websocket.send(Name + 'Release' + Id);
          }

          // --- HIER PASSIERT DIE WEITERLEITUNG ---
          // Wir suchen den Link (<A>-Tag) um den Button herum
          const linkElement = event.currentTarget.closest('a');
          if (linkElement && linkElement.getAttribute('href')) {
            const destination = linkElement.getAttribute('href');
          
            // Wichtig für das iPhone: 80ms warten, damit das 'Release'-Paket 
            // den ESP32 sicher erreicht, bevor die Seite stirbt.
            setTimeout(() => {
              window.location.href = destination;
            }, 80);
          }
        }, { passive: false });
      });
    });
  }

  function openB203(evt, TabName) {
    // Declare all variables
    var i, tabcontent, tablinks, tabname;

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

    tabname = document.getElementById(TabName).id;
    if (tabname == "B226Statustab"){
      console.log(tabname);
      getb226tabevent();
    } else if (tabname == "B203Setuptab") {
      console.log(tabname);
      getb203tabevent();
    } else if (tabname == "WIFIConfig") {
      console.log(tabname);
      getWIFIConfig();
    } else if (tabname == "B215Statustab") {
      console.log(tabname);
      getb215tabevent();
    } else if (tabname == "B285Statustab") {
      console.log(tabname);
      getb285tabevent();
    } else if (tabname == "B291Statustab") {
      console.log(tabname);
      getb291tabevent();
    } else if (tabname == "PortConfig") {
      console.log(tabname);
      loadData();
    } else if (tabname == "Config") {
      console.log(tabname);
      loadConfig();
    }
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

  function getB285Settings() {
    const rawdata = event.data.slice(3);
    const b285source = rawdata.charAt(0);
    document.getElementById("b285source").value = b285source;
    const getspeakers = rawdata.charAt(1);
    document.getElementById("getspeakers").value = getspeakers;
    const volume = rawdata.slice(2, 4);
    document.getElementById("volume").value = volume;
    const tunerstation = rawdata.slice(4, 6);
    document.getElementById("tunerstation").value = tunerstation;
    const stationid = rawdata.slice(6, 10);
    document.getElementById("stationid").value = stationid;
    const frequency = rawdata.slice(10, 15);
    document.getElementById("frequency").value = frequency;
  }

  function getB291Settings() {
    const rawdata = event.data.slice(3);
    const fiststatebyte = rawdata.charAt(0);
    const secondstatebyte = rawdata.charAt(1);
    const nibble1 = (fiststatebyte >> 4) & 0x0F;
    const nibble2 = fiststatebyte & 0x0F;
    const nibble3 = (secondstatebyte >> 4) & 0x0F;
    const nibble4 = secondstatebyte & 0x0F;
    if (nibble1 == 1) {
      document.getElementById("b291speed").value = nibble2;
      console.log(nibble2);
      document.getElementById("b291state").value = nibble3;
      console.log(nibble3);
    } else if (nibble1 == 5) {
    }
  }

  async function loadData() {
    try {
      const response = await fetch(`/portconfig.json?v=${Date.now()}`);
      if (!response.ok) throw new Error(`HTTP-Fehler!`);
      portconfigData = await response.json();
    
      // 1. Dropdown an den Zustand der JSON-Daten anpassen
      syncSelectField();
    
      // 2. Tabelle das erste Mal zeichnen
      renderTable(); 
    } catch (error) {
      console.error("Fehler beim Laden:", error);
    }
  }

  function updateValue(index, field, value) {
    portconfigData[index][field] = value;
  }

  async function saveData() {
    try {
      const response = await fetch('/api/save-data', { // Ersetze dies mit deinem echten Speicher-Pfad/API
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(portconfigData) // Schickt den aktuellen Zustand aus dem Browser-Speicher
      });

      if (!response.ok) throw new Error("Fehler beim Speichern auf dem Server");
    
      alert("Konfiguration erfolgreich gespeichert!");
    
      // Nach dem Speichern laden wir die Daten neu, um sicherzustellen, 
      // dass Server und UI zu 100% synchron sind.
      await loadData(); 

      window.location.href = '/'; // Leitet zur Hauptseite weiter

    } catch (error) {
      console.error("Speicherfehler:", error);
      alert("Fehler beim Sichern der Daten.");
    }
  }

  async function loadConfig() {
    const tbody = document.getElementById('configTable');
    if (!tbody) {
        console.error("Meldung: Das Element 'configTable' wurde im HTML nicht gefunden!");
        return; 
    }
    
    try {
        console.log("Meldung: Klick registriert. Starte Fetch-Anfrage an den ESP...");
        const response = await fetch('/api/config?_cb=' + Date.now());
        if (!response.ok) throw new Error('Laden fehlgeschlagen');
        
        configData = await response.json();
        console.log("Meldung: JSON erfolgreich empfangen. Anzahl Einträge:", configData.length);
        
        tbody.innerHTML = '';
        
        configData.forEach((item, index) => {

            const ignoriertBtnIDs = ["falseIR", "trueIR", "amppoweron", "tun20", "tun21", "tun22", "tun23", "tun24", "tun25", "tun26", "tun27", "tun28", "tun29", "none"];
            
            if (item.btnID && ignoriertBtnIDs.includes(item.btnID)) {
                return; // Überspringt diese Zeile und geht zur nächsten
            }
            const tr = document.createElement('tr');
            
            // 1. HEX-Formatierung des IR-Codes (Ihre bestehende Logik)
            let rawCode = item.irRecvCode;
            let displayCode = "";

            if (typeof rawCode === 'number') {
                displayCode = '0x' + rawCode.toString(16).toUpperCase();
            } 
            else if (typeof rawCode === 'string') {
                rawCode = rawCode.trim();
                if (rawCode.toLowerCase().startsWith('0x')) {
                    displayCode = '0x' + rawCode.substring(2).toUpperCase();
                } else {
                    let parsed = parseInt(rawCode, 10);
                    displayCode = isNaN(parsed) ? '0x0' : '0x' + parsed.toString(16).toUpperCase();
                }
            } else {
                displayCode = '0x0';
            }

            // 2. NEU: Checkbox für cmdFlag prüfen (nur wenn serCmd nicht null ist)
            let checkboxHtml = "";
            if (item.serCmd !== null && item.serCmd !== undefined) {
                // Aktiviert, wenn cmdFlag "1", 1 oder true ist
                const isChecked = (item.cmdFlag === "1" || item.cmdFlag === 1 || item.cmdFlag === true) ? "checked" : "";
                checkboxHtml = `
                    <input type="checkbox" 
                           ${isChecked} 
                           data-index="${index}" 
                           class="config-checkbox">
                `;
            }

            // 3. HTML Struktur der Zeile befüllen (Inklusive der neuen 3. Spalte)
            tr.innerHTML = `
                <td class="btnID-text">${item.btnID || '-'}</td>
                <td>
                    <input type="text" class="irRecvCode-input" data-index="${index}" value="${displayCode}">
                </td>
                <td style="text-align: center; vertical-align: middle;">
                    ${checkboxHtml}
                </td>
            `;
            tbody.appendChild(tr);
        });

        console.log("Meldung: Tabelle wurde erfolgreich im HTML sichtbar befüllt!");

    } catch (error) {
        console.error("Meldung: Fehler im try-catch Block aufgetreten:", error);
        alert('Fehler beim Laden: ' + error.message);
    }
}

  async function saveConfig() {
    // 1. IR-Codes verarbeiten (Ihre bestehende optimierte Logik)
    const inputs = document.querySelectorAll('#configTable .irRecvCode-input');
    inputs.forEach(input => {
        const index = parseInt(input.getAttribute('data-index'), 10);
        const val = input.value.trim();
        
        if (!configData || configData[index] === undefined) {
            return; 
        }

        let parsedNumber = 0;
        if (val.toLowerCase().startsWith('0x')) {
            parsedNumber = parseInt(val, 16); 
        } else {
            parsedNumber = parseInt(val, 10); 
        }

        if (isNaN(parsedNumber)) {
            configData[index].irRecvCode = "0x0";
        } else {
            configData[index].irRecvCode = "0x" + parsedNumber.toString(16).toUpperCase();
        }
    });

    // 2. NEU: Checkboxen für cmdFlag verarbeiten
    const checkboxes = document.querySelectorAll('#configTable .config-checkbox');
    checkboxes.forEach(checkbox => {
        const index = parseInt(checkbox.getAttribute('data-index'), 10);
        
        // Sicherstellen, dass das Objekt an diesem Index existiert
        if (!configData || configData[index] === undefined) {
            return; 
        }
        
        // Setzt "1" wenn die Checkbox aktiv ist, sonst "0"
        configData[index].cmdFlag = checkbox.checked ? "1" : "0";
    });

    // 3. Daten an den ESP senden (Ihre bestehende fetch-Logik)
    try {
        const response = await fetch('/api/config', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(configData)
        });
        if (response.ok) {
            alert('Einstellungen erfolgreich aktualisiert!');
        } else {
            alert('Fehler beim Speichern.');
        }
    } catch (error) {
        alert('Netzwerkfehler: ' + error.message);
    }
}