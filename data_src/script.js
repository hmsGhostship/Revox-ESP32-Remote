
// @ts-ignore
let ws;
const gateway = `ws://${window.location.hostname}/ws`;
/** @type {Object[]} */
let portconfigData = [];
/** @type {Object[]} */
 // Globale Variable, um alle restlichen Daten im Hintergrund zu behalten
let configData = [];

// Zwei globale Sperren ganz oben in Ihrer script.js hinzufügen (falls noch nicht da)
let isTouchProcessing = false;
let globalTouchTimeout = null;

const HIDE_AMP_TUN_WITHOUT_RECEIVER = true; 

// \u00e4 = ä, \u00f6 = ö, \u00fc = ü, \u00df = ß

window.addEventListener('load', onLoad);
//window.addEventListener('DOMContentLoaded', loadConfig);

function onLoad() {
  // ==================================================
  // 1. GLOBALE INITIALISIERUNG (Exakt dein Original)
  // ==================================================
  initWebSocket();
  getButton();
  setIRstate();
  setPorts();
  setBibus();

  // ==================================================
  // 2. DYNAMISCHE SEITEN- UND GRUPPENERKENNUNG
  // ==================================================
  const path = window.location.pathname;
  const page = path.split("/").pop().toLowerCase(); 

  console.log("[Routing] Aktuelle HTML-Seite geladen: " + page);

  // KORREKTUR: Erkennt deine Seiten der Receiver-Gruppe (inkl. b203_rec.html / b203_recv.html)
  const isReceiverWorld = page.includes("_recv") || page.includes("_rec") || page === "receiver.html";
  const isAmplifierWorld = !isReceiverWorld;

  // ==================================================
  // 3. LOGIK FÜR DIE RECEIVER-WELT (B285 aktiv)
  // ==================================================
  if (isReceiverWorld) {
    console.log("[Routing] Modus: Receiver-Welt (B285).");
    
    // Wenn die B203-Seite in der Receiver-Gruppe geöffnet wird (b203_rec.html / b203_recv.html)
    if (page.startsWith("b203")) {
      console.log("[Routing] Lade Setup-Daten für B203 (Receivergruppe).");
      setb203();
      getb203(); 
      setEventb203();
      setDateb203();
      setTimeb203();
      callEventb203();
      delEventb203();
      testEventb203();

      // KORREKTUR: Warte ganz kurz, bis der WebSocket OPEN ist, dann "klicken"
      setTimeout(() => {
        console.log("[Routing] Automatischer Erstabruf beim Betreten der B203-Seite");
        document.getElementById('b203_get')?.click();
      }, 150);
    } 
    // Für alle anderen echten Receiver-Seiten
    else {
      getb285();
      setB285Speakers();

      if (page.startsWith("cd_recv")) {
        getb226();
        setTimeout(() => document.getElementById('b226_get')?.click(), 150); // Verzögerter Erstabruf
      } else if (page.startsWith("tape1_recv")) {
        getb215();
        setTimeout(() => document.getElementById('b215_get')?.click(), 150); // Verzögerter Erstabruf
      } else if (page.startsWith("receiver")) {
        getb285();
        setTimeout(() => document.getElementById('b285_get')?.click(), 150); // Verzögerter Erstabruf
      }
    }
  } 

  // ==================================================
  // 4. LOGIK FÜR DIE BAUSTEIN-WELT (Verstärker B251 + Einzelgeräte)
  // ==================================================
  else if (isAmplifierWorld) {
    console.log("[Routing] Modus: Baustein-Welt (B251 / Einzelgeräte).");

    if (page === "amplifier.html") {
      console.log("[Routing] Lade Hauptfunktionen für B251 Verstärker.");
    } 
    else if (page === "cd.html") {
      getb226();
      setTimeout(() => document.getElementById('b226_get')?.click(), 150); // Verzögerter Erstabruf
    } else if (page === "tape1.html") {
      getb215();
      setTimeout(() => document.getElementById('b215_get')?.click(), 150); // Verzögerter Erstabruf
    }
    else if (page === "b203.html" || page === "index.html" || page === "") {
      console.log("[Routing] Lade Setup-Daten für B203.");
      setb203();
      getb203(); 
      setEventb203();
      setDateb203();
      setTimeb203();
      callEventb203();
      delEventb203();
      testEventb203();

      // KORREKTUR: Warte ganz kurz, bis der WebSocket OPEN ist, dann "klicken"
      setTimeout(() => {
        console.log("[Routing] Automatischer Erstabruf beim Betreten der B203-Bausteinseite");
        document.getElementById('b203_get')?.click();
      }, 150);
    }
  }

  // ==================================================
  // 5. AUTOMATISCHER TAB-TRIGGER (Nur bei Status- und Setup-Tabs!)
  // ==================================================
  console.log("[Tabs] Initialisiere Klick-Erkennung für .tablinks Buttons.");
  document.querySelectorAll('.tablinks').forEach(tabButton => {
    tabButton.addEventListener('click', (event) => {
      const onClickAttr = event.target.getAttribute('onclick') || "";
      
      console.log("[Tabs] Tab-Button geklickt. OnClick-Inhalt: " + onClickAttr);

      if (onClickAttr.includes("B226") && onClickAttr.includes("Status")) {
        console.log("[Tabs] CD Status-Tab geöffnet -> Trigger Laden");
        document.getElementById('b226_get')?.click();
      } 
      else if (onClickAttr.includes("B215") && onClickAttr.includes("Status")) {
        console.log("[Tabs] Tape Status-Tab geöffnet -> Trigger Laden");
        document.getElementById('b215_get')?.click();
      } 
      else if (onClickAttr.includes("B285") && (onClickAttr.includes("Status") || onClickAttr.includes("B285tab"))) {
        console.log("[Tabs] Receiver Status-Tab geöffnet -> Trigger Laden");
        document.getElementById('b285_get')?.click();
      }
      else if (onClickAttr.includes("B203Setuptab")) {
        console.log("[Tabs] B203 Setup-Tab geöffnet -> Trigger Laden");
        document.getElementById('b203_get')?.click();
      }
    });
  });
}

function setBibus() {
  // Event-Listener für gegenseitige Beeinflussung der Checkboxen
document.getElementById('configTable')?.addEventListener('change', (event) => {
    const target = event.target;
    const index = target.getAttribute('data-index');
    if (!index) return;

    // Fall 1: BiBus Active wurde geklickt
    if (target.classList.contains('bibus-checkbox')) {
        if (target.checked) {
            // Suchen der Flag (Serial) Checkbox in derselben Zeile
            const serialCheckbox = document.querySelector(`#configTable .config-checkbox[data-index="${index}"]`);
            if (serialCheckbox) {
                serialCheckbox.checked = false; // Deaktivieren
            }
        }
    }

    // Fall 2: Flag (Serial) wurde geklickt
    if (target.classList.contains('config-checkbox')) {
        if (target.checked) {
            // Suchen der BiBus Active Checkbox in derselben Zeile
            const bibusCheckbox = document.querySelector(`#configTable .bibus-checkbox[data-index="${index}"]`);
            if (bibusCheckbox) {
                bibusCheckbox.checked = false; // Deaktivieren
            }
        }
    }
});
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

function renderTable() {
  const tbody = document.getElementById('table-body');
  if (!tbody) return;

  tbody.innerHTML = portconfigData.map((item, index) => {
    const safeName = escapeHtml(item.name || '');
    const safeDescr = escapeHtml(item.descr || '');
    
    // Konvertiert den aktuellen Wert sicher zu einer Zahl für den Vergleich
    const currentOutValue = (item.out === 'no' || item.out === 'Nicht verf&uuml;gbar') ? 'no' : Number(item.out);
    
    // Generiert die Optionen von 0 bis 9 (Vergleich mittels Number)
    const options = Array.from({ length: 10 }, (_, i) => `
      <option value="${i}" ${currentOutValue === i ? 'selected' : ''}>${i}</option>
    `).join('');

    return `
      <tr data-index="${index}">
        <td><input class="portselect" type="text" value="${safeName}" autocomplete="off" disabled></td>
        <td><input class="portselect" type="text" value="${safeDescr}" autocomplete="off" disabled></td>
        <td>
          <select class="portselect" data-action="update-out">
            <option value="no" ${currentOutValue === 'no' ? 'selected' : ''}>Nicht verf&uuml;gbar</option>
            ${options}
          </select>
        </td>
        <td><input type="checkbox" ${item.feedback ? 'checked' : ''} disabled></td>
      </tr>
    `;
  }).join('');
}

// Überwacht Änderungen auf der gesamten Seite, filtert aber gezielt Ihre Port-Dropdowns heraus
document.addEventListener('change', (event) => {
const target = event.target;
  
  // Prüfen, ob das geänderte Element unser Port-Select ist
  if (target && target.matches('select[data-action="update-out"]')) {
    // Suchen der Tabellenzeile (tr), um den Index zu bekommen
    const row = target.closest('tr');
    if (!row) return;
    const rowIndex = row.getAttribute('data-index');
    const rawValue = target.value;
    // Wert umwandeln: Wenn 'no', dann als String lassen, sonst als echte Zahl
    const finalValue = rawValue === 'no' ? 'no' : Number(rawValue);
    console.log(`[Event] Dropdown geändert! Zeile: ${rowIndex}, Neuer Wert:`, finalValue);
    // Wert im Array überschreiben
      if (typeof updateValue === 'function') {
        updateValue(Number(rowIndex), 'out', finalValue);
      }
  }
});

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
  ws = new WebSocket(gateway);
  ws.onopen    = onOpen;
  ws.onclose   = onClose;
  ws.onmessage = onMessage; // <-- add this line
}
  
// When websocket is established, call the getReadings() function
function onOpen(event) {
  console.log('Connection opened');
  ws.send('get_data');
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}


function onMessage(event) {
  console.log("WebSocket empfangen:", event.data);
  
  const rawString = event.data;
  if (!rawString || typeof rawString !== 'string') return;

  // Extrahiert zwei Zeichen ab Index 1 (z.B. aus "1031141" wird "03")
  const Identifier = rawString.slice(1, 3);
  const idNum = Number(Identifier);
  
  // Ermittelt die allererste Ziffer der ID (z.B. bei ID 52 -> Klasse 5)
  const firstDigit = Math.floor(idNum / 10);

  // 1. Zuordnung von ReVox-Geräte-IDs zu ihren Verarbeitungsfunktionen
  const deviceActions = {
    3: getB285Settings, // ID 3  = B285 Receiver
    4: getB215Settings, // ID 4  = B215 Tape
    6: getB226Settings, // ID 6  = B226 CD-Player
    8: getB291Settings  // ID 8  = B291 Plattenspieler
    // Hier kannst du später einfach neue Geräte eintragen, z.B.:
    // 1: getPR99Settings,
    // 2: getA725Settings
    // 5: getB225_2Settings
    // 7: getA725_2Settings
  };

  // 2. Prüfen, ob für die ID eine direkte Geräte-Funktion existiert
  if (deviceActions[idNum]) {
    deviceActions[idNum](rawString);
  } 
  // 3. REVOX B203 Spezial-Bedingung (Klasse 5x oder 9x)
  else if (firstDigit === 5 || firstDigit === 9) { 
    console.log(`B203 erkannt mit ID ${idNum} (Klasse ${firstDigit}x), rufe getB203Settings auf...`);
    getB203Settings(rawString); 
  }
}

function setIRstate() {
  document.getElementById('IR')?.addEventListener('change', (event) => {
    const Id = event.target.id;
    const Value = event.target.checked;
    const Name = event.target.name;
    if (ws.readyState === WebSocket.OPEN) {
      console.log(Name + Value + Id);
      ws.send(Name + Value + Id);
    }
  });
}

// 1. Die verarbeitende Funktion komplett eigenständig machen
function handleSpeakerChange(event) {
  // Verhindert, dass andere Skripte oder Tab-Systeme dieses Event blockieren
  event.stopPropagation(); 

  if (event.target && event.target.id === 'speakers') {
    const Value = event.target.value;
    const Name = event.target.name;

    if (ws && ws.readyState === WebSocket.OPEN) {
      console.log("[WebSocket ERFOLG] Sende direkt: " + Name + Value);
      ws.send(Name + Value);
    } else {
      console.error("[WebSocket FEHLER] Verbindung im Tab nicht offen!");
    }
  }
}

// 2. Die Initialisierung direkt an das Element binden
function setB285Speakers() {
  const el = document.getElementById('speakers');
  if (!el) return;

  // Alte Listener entfernen, um Dopplungen im Tab zu vermeiden
  el.removeEventListener('change', handleSpeakerChange);
  el.removeEventListener('input', handleSpeakerChange);
  
  // 'input' reagiert im Tab oft schneller und sicherer vor Blockaden als 'change'
  el.addEventListener('change', handleSpeakerChange);
  el.addEventListener('input', handleSpeakerChange);
}

function setB285Volume() {
  let lastValue = ""; // Speichert den zuletzt gesendeten Wert
  // 'input' feuert kontinuierlich während des Ziehens
  document.getElementById('volumeSlider')?.addEventListener('change', (event) => {
    const Value = event.target.value;
    const Name = event.target.name;
    // Nur senden, wenn sich die Zahl wirklich geändert hat (schont den WebSocket)
    if (Value !== lastValue) {
      lastValue = Value; // Aktualisieren
      if (ws.readyState === WebSocket.OPEN) {
        console.log(Name + "V" + Value);
        ws.send(Name + "V" + Value);
      }
    }
  });
}

function setPorts() {
  document.getElementById('b203setport')?.addEventListener('click', () => {
    
    // ABSICHERUNG: Wenn das globale Array leer oder ungültig ist, brechen wir ab
    if (!Array.isArray(portconfigData) || portconfigData.length === 0) {
      alert("Fehler: Keine Konfigurationsdaten zum Speichern vorhanden!");
      return;
    }

    // TYP-SICHERUNG FÜR C++: Wir bereinigen die Daten kurz, damit ArduinoJson 
    // die richtigen Datentypen (Strings für "out" und Booleans für "feedback") erhält.
    const bereinigteDaten = portconfigData.map(item => {
      return {
        name: item.name || "",
        descr: item.descr || "",
        out: item.out === 'no' ? 'no' : String(item.out), // Konvertiert Zahlen wie 4 zu "4"
        feedback: item.feedback === true || item.feedback === 1 // Garantiert echtes true/false
      };
    });

    // In JSON-String umwandeln
    const jsonErgebnis = JSON.stringify(bereinigteDaten);
    console.log("Sende Port-JSON an C++ Server:", jsonErgebnis);

    // POST-Request an deinen C++ Server senden
    fetch('/api/save-data', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: jsonErgebnis
    })
    .then(response => {
      if (response.ok) {
        console.log("Daten erfolgreich im LittleFS gespeichert.");
        // WEITERLEITUNG: Seite neu laden, C++ liefert die korrekte Oberfläche aus
        window.location.href = "/";
      } else {
        console.error("Fehler beim Speichern:", response.statusText);
        alert("Fehler beim Speichern der Einstellungen!");
      }
    })
    .catch(error => {
      console.error("Netzwerkfehler:", error);
      alert("Verbindung zum Revox-Server fehlgeschlagen!");
    });
  });
}

function setb203() {
  document.getElementById('b203_set')?.addEventListener('click', (event) => {
    const Name = event.target.name;
    const language = document.querySelector('select[id="language"]').value;
    const easy = document.querySelector('select[id="easy"]').value;
    const timer = document.querySelector('select[id="timer"]').value;
    const poweron = document.querySelector('select[id="poweron"]').value;
      if (ws.readyState === WebSocket.OPEN) {
        console.log( Name + "0S" + language + easy + timer + poweron );
        ws.send( Name + "0S" + language + easy + timer + poweron );
    }
  });
}

function getb215() {
  document.getElementById('b215_get')?.addEventListener('click', (event) => {
    const Name = event.target.name;
    if (ws.readyState === WebSocket.OPEN) {
    console.log( "tape1X" );
    ws.send("tape1X");
    }
  });
}
  

function getb226() {
  document.getElementById('b226_get')?.addEventListener('click', (event) => {
    const Name = event.target.name;
    if (ws.readyState === WebSocket.OPEN) {
      console.log( "cdplayerX" );
      ws.send("cdplayerX");
    }
  });
}

function getb285() {
  document.getElementById('b285_get')?.addEventListener('click', (event) => {
  const Name = event.target.name;
    if (ws.readyState === WebSocket.OPEN) {
      console.log( "receiverX" );
      ws.send("receiverX");
    }
  });
}

function getb203() {
  // Übergebe 'event' in die Klammer, damit target.name funktioniert
  document.getElementById('b203_get')?.addEventListener('click', (event) => {
    const Name = event.target.name; // Ergibt jetzt korrekt "getsettings"
    if (ws.readyState === WebSocket.OPEN) {
      console.log(Name + "0X");
      ws.send(Name + "0X"); // Sendet "getsettings0X" an den Arduino
    }
  });
}

function getb291() {
  document.getElementById('b291_get')?.addEventListener('click', (event) => {
  const Name = event.target.name;
    if (ws.readyState === WebSocket.OPEN) {
      console.log( "phonoX" );
      ws.send( "phonoX");
    }
  });
}

function getb291tabevent() {
  if (ws.readyState === WebSocket.OPEN) {
    console.log( "phonoX" );
    ws.send("phonoX");
  }
}

function getb226tabevent() {
  if (ws.readyState === WebSocket.OPEN) {
    console.log( "cdplayerX" );
    ws.send("cdplayerX");
  }
}

function getb215tabevent() {
  if (ws.readyState === WebSocket.OPEN) {
    console.log( "tape1X" );
    ws.send("tape1X");
  }
}

function getb203tabevent() {
  if (ws.readyState === WebSocket.OPEN) {
    console.log( "getsettings0X" );
    ws.send( "getsettings0X");
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
  if (ws.readyState === WebSocket.OPEN) {
    console.log( "receiverX" );
    ws.send("receiverX");
  }
}



function setDateb203() {
  document.getElementById('setDate')?.addEventListener('click', (event) => {
    const Name = event.target.name;
    const setdate = document.querySelector("#set_date").value;
    let millenShort  = setdate.slice(2)
    let splitdate = millenShort.replace(/-/g, "");
    const swaped = swapPairs(splitdate);
    if (ws.readyState === WebSocket.OPEN) {
      console.log( Name + "0D" + swaped );
      ws.send( Name + "0D" + swaped );
    }
  });
}

function setTimeb203() {
  document.getElementById('setTime')?.addEventListener('click', (event) => {
  const Name = event.target.name;
  const setdate = document.querySelector("#set_time").value;
  let formtime = setdate.replace(/:/g, "");
    if (ws.readyState === WebSocket.OPEN) {
      console.log( Name + "0T" + formtime );
      ws.send( Name + "0T" + formtime );
    }
  });
}

function callEventb203() {
  document.getElementById('b203callevent')?.addEventListener('click', (event) => {
  const Name = event.target.name;
  const b203call = document.querySelector('select[id="b203evn"]').value;
    if (ws.readyState === WebSocket.OPEN) {
      console.log( Name + "0C" + b203call );
      ws.send( Name + "0C" + b203call );
    }
  });
}

function delEventb203() {
  document.getElementById('b203delevent')?.addEventListener('click', (event) => {
  const Name = event.target.name;
  const b203del = document.querySelector('select[id="b203evn"]').value;
    if (ws.readyState === WebSocket.OPEN) {
      console.log( Name + "0E" + b203del + "E" );
      ws.send( Name + "0E" + b203del + "E" );
    }
  });
}

function testEventb203() {
  document.getElementById('b203testevent')?.addEventListener('click', (event) => {
  const Name = event.target.name;
  const b203test = document.querySelector('select[id="b203evn"]').value;
    if (ws.readyState === WebSocket.OPEN) {
      console.log( Name + "0V" + b203test  );
      ws.send( Name + "0V" + b203test );
    }
  });
}

function setEventb203() {
  document.getElementById('b203setevent')?.addEventListener('click', (event) => {
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
    if (ws.readyState === WebSocket.OPEN) {
      console.log( Name + "0E" + b203evn + datatype + weekdayString + signalsource + sourceadd + outputString + formevstarttime + formevstoptime);
      ws.send( Name + "0E" + b203evn + datatype + weekdayString + signalsource + sourceadd + outputString + formevstarttime + formevstoptime );
    }
  });
}

function getButton() {
  const buttons = document.querySelectorAll('.button:not(.js-bound), .misc_button:not(.js-bound), .power_btn:not(.js-bound)');
  
  buttons.forEach(btn => {
    // Slider und Selektoren aus der Hardwaretasten-Logik aussortieren
    if (btn.tagName === 'INPUT' || btn.tagName === 'SELECT' || btn.classList.contains('slide') || btn.id === 'volumeSlider') {
      return; 
    }

    btn.classList.add('js-bound');
    
    // --- HILFSFUNKTION: DRÜCKEN (PUSH) ---
    const handlePush = (element) => {
      if (isTouchProcessing) return; 
      isTouchProcessing = true;
      element.classList.add('is-pressed');
      const Id = element.getAttribute('data-id') || element.id; 

      if (ws.readyState === WebSocket.OPEN) {
        console.log("Sende: buttonPush" + Id);
        ws.send("buttonPush" + Id);
      }
    };

    // --- HILFSFUNKTION: LOSLASSEN (RELEASE) ---
    const handleRelease = (element) => {
      element.classList.remove('is-pressed');
      const Id = element.getAttribute('data-id') || element.id;
      const linkElement = element.closest('a') || element;
      const targetUrl = linkElement ? linkElement.getAttribute('href') : null;

      if (ws.readyState === WebSocket.OPEN) {
        console.log("Sende: buttonRelease" + Id);
        ws.send("buttonRelease" + Id);
        
        if (targetUrl && targetUrl !== "#" && targetUrl !== "") {
          setTimeout(() => {
              sicherLeiten(targetUrl);
              isTouchProcessing = false;
          }, 150);
        } else {
          isTouchProcessing = false;
        }
      } else {
        if (targetUrl && targetUrl !== "#" && targetUrl !== "") {
          sicherLeiten(targetUrl);
        }
        isTouchProcessing = false;
      }
    };

    // LISTENERS FÜR HARDWARE-BUTTONS ZUWEISEN
    btn.addEventListener('touchstart', (event) => {
      event.preventDefault(); 
      handlePush(event.currentTarget);
    }, { passive: false });

    btn.addEventListener('touchend', (event) => {
      event.preventDefault(); 
      handleRelease(event.currentTarget);
    }, { passive: false });

    btn.addEventListener('mousedown', (event) => {
      if (event.button === 0) { 
        event.preventDefault(); 
        handlePush(event.currentTarget);
      }
    });

    btn.addEventListener('mouseup', (event) => {
      if (event.button === 0) {
        event.preventDefault(); 
        handleRelease(event.currentTarget);
      }
    });

    btn.addEventListener('click', (event) => {
      event.preventDefault();
    });
  });

  // ====================================================================
  // DIREKTE EVENT-BINDUNG FÜR DEN VOLUME-SLIDER (EXAKT FÜR DEINEN ESP32)
  // ====================================================================
  const volSlider = document.getElementById('volumeSlider');
  if (volSlider && !volSlider.classList.contains('js-bound')) {
    volSlider.classList.add('js-bound');

    // Das 'change'-Event feuert exakt beim Loslassen des Sliders
    volSlider.addEventListener('change', (event) => {
      // Erzwingt, dass die Zahl immer 2-stellig ist (z.B. aus 5 wird "05")
      const currentVolume = String(event.target.value).padStart(2, '0');
      
      // Das exakte Format für deinen Server: "volSliderV" (10 Zeichen) + "xx"
      const espMessage = "volSliderV" + currentVolume;

      console.log("[Slider-Aktion] Sende via WS: " + espMessage);

      if (ws.readyState === WebSocket.OPEN) {
        ws.send(espMessage); // Schickt "volSliderV50" an den C++ Code
      }
    });
  }
  

  // ====================================================================
  // LOGIK FÜR ALLE EINSTELLUNGS-, CONFIG- & TIMER-BUTTONS (.set_button)
  // ====================================================================
  const setButtons = document.querySelectorAll('.set_button:not(.js-bound), .set_event_button:not(.js-bound)');
  
  setButtons.forEach(btn => {
    btn.classList.add('js-bound');
    let buttonLocked = false;

    const handleSetButtonClick = (element) => {
      if (buttonLocked) return;
      buttonLocked = true;
      element.classList.add('is-pressed');
      
      const inlineOnClick = element.getAttribute('onclick');
      
      if (inlineOnClick) {
        console.log(`[Einstellung] Inline-onclick im HTML erkannt: ${inlineOnClick}`);
      } else {
        const functionName = element.getAttribute('name');
        
        const nameMapping = {
          'setup': 'setb203',
          'getsettings': 'getb203',
          'getb285settings': 'getb285',
          'getb226settings': 'getb226',
          'getb215settings': 'getb215',
          'setDate': 'setDateb203',
          'setTime': 'setTimeb203',
          'setEvent': 'setEventb203',
          'call': 'callEventb203',      
          'delete': 'delEventb203',     
          'testEvent': 'testEventb203'
        };

        let targetFunction = nameMapping[functionName] || functionName;

        if (typeof window[targetFunction] !== 'function') {
          if (typeof window[targetFunction.toLowerCase()] === 'function') {
            targetFunction = targetFunction.toLowerCase();
          }
        }

        if (typeof window[targetFunction] === 'function') {
          console.log(`[Einstellung] Button ausgeführt: ${targetFunction}()`);
          window[targetFunction](); 
        } else {
          console.warn(`[Einstellung] Funktion ${targetFunction}() nicht gefunden.`);
        }
      }

      setTimeout(() => {
        element.classList.remove('is-pressed');
        buttonLocked = false; 
      }, 150);
    };

    btn.addEventListener('touchstart', (event) => {
      if (!btn.getAttribute('onclick')) {
        event.preventDefault(); 
      }
      handleSetButtonClick(event.currentTarget);
    }, { passive: true });

    btn.addEventListener('mousedown', (event) => {
      if (event.button === 0) { 
        handleSetButtonClick(event.currentTarget);
      }
    });
  });
}

// Komplett neue Funktion – verarbeitet NUR Text, kein HTML-Element!
function sicherLeiten(zielUrl) {
  if (zielUrl) {
    window.location.href = zielUrl;
  }
}

function executeNavigation(element) {
  const linkElement = element.closest('a');
  if (linkElement && linkElement.getAttribute('href')) {
    window.location.href = linkElement.getAttribute('href');
  }
}

function openB203(evt, TabName) {
  // 1. Alle Tab-Inhalte ausblenden
  const tabcontent = document.getElementsByClassName("tabcontent");
  for (let i = 0; i < tabcontent.length; i++) {
    tabcontent[i].style.display = "none";
  }

  // 2. "active"-Klasse von allen Tab-Links entfernen
  const tablinks = document.getElementsByClassName("tablinks");
  for (let i = 0; i < tablinks.length; i++) {
    tablinks[i].className = tablinks[i].className.replace(" active", "");
  }

  // 3. Aktuellen Tab anzeigen und Button aktivieren
  const currentTab = document.getElementById(TabName);
  if (!currentTab) return; // Sicherheits-Check, falls TabName nicht existiert

  currentTab.style.display = "block";
  evt.currentTarget.className += " active";

  // 4. Zuordnung von Tab-IDs zu den entsprechenden Funktionen (Lookup-Table)
  const tabActions = {
    "B226Statustab": getb226tabevent,
    "B203Setuptab": getb203tabevent,
    "WIFIConfig": getWIFIConfig,
    "B215Statustab": getb215tabevent,
    "B285Statustab": getb285tabevent,
    "B291Statustab": getb291tabevent,
    "PortConfig": loadData,
    "Config": loadConfig
  };

  const tabname = currentTab.id;
  
  // 5. Funktion dynamisch ausführen, falls sie im Objekt existiert
  if (tabActions[tabname]) {
    console.log(tabname);
    tabActions[tabname](); // Führt die gemappte Funktion aus
  }
}

  function swapPairs(str) {
    if (str.length < 6) return "String zu kurz";
    let p1 = str.substring(0, 2);
    let p2 = str.substring(2, 4);
    let p3 = str.substring(4, 6);
    return p3 + p2 + p1;
  }

function getB203Settings(incomingData) {
  const rawString = incomingData || event.data;
  console.log("Verarbeite B203 Daten:", rawString);
  
  // Datum extrahieren (ddmmyy ab Index 3)
  const datestr = rawString.slice(3, 9); 
  const day = datestr.substring(0, 2);     
  const month = datestr.substring(2, 4);   
  const year = datestr.substring(4, 6);    
  const yearlong = "20" + year;            
  
  // Uhrzeit extrahieren (hhmmss ab Index 9)
  const timestr = rawString.slice(9, 15);
  const hour = timestr.substring(0, 2);    
  const minute = timestr.substring(2, 4);  
  const second = timestr.substring(4, 6);  
  
  const dateInput = document.getElementById("set_date");
  const timeInput = document.getElementById("set_time");
  if (dateInput) dateInput.value = `${yearlong}-${month}-${day}`; 
  if (timeInput) timeInput.value = `${hour}:${minute}:${second}`; 

  // Setup-Dropdowns befüllen
  const langElem = document.getElementById("language");
  if (langElem) langElem.value = rawString.charAt(15);
  
  const easyElem = document.getElementById("easy");
  if (easyElem) easyElem.value = rawString.charAt(16);
  
  const timerElem = document.getElementById("timer");
  if (timerElem) timerElem.value = rawString.charAt(17);
  
  const pwrElem = document.getElementById("poweron");
  if (pwrElem) pwrElem.value = rawString.charAt(18);
  
  // IR-Schalter
  const irCheckbox = document.getElementById("IR");
  if (irCheckbox) {
      irCheckbox.checked = (rawString.charAt(19) == "0");
  }
}

  function getB215Settings(incomingData) {
  // Holt die Daten sicher entweder aus dem Parameter oder dem globalen Event
  const rawString = incomingData || (typeof event !== 'undefined' ? event.data : '');
  if (!rawString) return;

  // WICHTIG: Hier muss rawString stehen, nicht event.data!
  const rawdata = rawString.slice(3); 
  
  const functions = rawdata.charAt(0);
  const functionsElem = document.getElementById("functions");
  if (functionsElem) functionsElem.value = functions;

  const addfunctions = rawdata.charAt(1);
  const addfunctionsElem = document.getElementById("addfunctions");
  if (addfunctionsElem) addfunctionsElem.value = addfunctions;

  const cuestate = rawdata.charAt(2);
  const cuestateElem = document.getElementById("cuestate");
  if (cuestateElem) cuestateElem.value = cuestate;

  const tapecountermm = rawdata.slice(3, 5);
  const tapecounterss = rawdata.slice(5, 7);
  const tapecounter = (tapecountermm + ":" + tapecounterss);
  
  const bandzaehlerElem = document.getElementById("bandzaehler");
  if (bandzaehlerElem) bandzaehlerElem.value = tapecounter;
}

function getB226Settings(incomingData) {
  // Holt die Daten direkt aus dem Parameter
  const rawString = incomingData || event.data;
  console.log("Verarbeite B226 Daten:", rawString);

  // 1. Gerätestatus extrahieren (Index 3, 1 Zeichen -> z.B. "2")
  const b226stateId = rawString.charAt(3); 
  const stateElem = document.getElementById("b226state");
  if (stateElem) {
    stateElem.value = b226stateId; // Findet im HTML die <option value="2">Play</option>
  }
  
  // 2. Track-Nummer extrahieren (Index 4 und 5 -> z.B. "01")
  const tracknumber = rawString.slice(4, 6);
  const trackElem = document.getElementById("tracknumber");
  if (trackElem) trackElem.value = tracknumber;
  
  // 3. Index-Nummer extrahieren (Index 6 und 7 -> z.B. "01")
  const indexnumber = rawString.slice(6, 8);
  const indexElem = document.getElementById("indexnumber");
  if (indexElem) indexElem.value = indexnumber;
  
  // 4. Abgelaufene Zeit extrahieren (Index 8 bis 12 -> "0019")
  const elapsedtimemm = rawString.slice(8, 10);  // "00"
  const elapsedtimess = rawString.slice(10, 12); // "19"
  const elapsedtime = elapsedtimemm + ":" + elapsedtimess;
  const elapsedElem = document.getElementById("elapsedtime");
  if (elapsedElem) elapsedElem.value = elapsedtime;
  
  // 5. Verbleibende Zeit extrahieren (Index 12 bis 16 -> "6700")
  const remainingtimemm = rawString.slice(12, 14); // "67"
  const remainingtimess = rawString.slice(14, 16); // "00"
  const remainingtime = remainingtimemm + ":" + remainingtimess;
  const remainingElem = document.getElementById("remainingtime");
  if (remainingElem) remainingElem.value = remainingtime;
}

 function getB285Settings(incomingData) {
  if (!incomingData || typeof incomingData !== 'string') return;
  
  console.log("Verarbeite B285 Daten:", incomingData);

  // Wir schneiden die ersten 3 Zeichen (Kanal + Geräteidentifier, z.B. "103") ab
  const rawdata = incomingData.slice(3);
  
  // 1. Signalquelle (C) -> 1 Zeichen an Index 0
  const b285source = rawdata.charAt(0);
  const sourceElem = document.getElementById("b285source");
  if (sourceElem) sourceElem.value = b285source;
  
  // 2. Lautsprecher (D) -> 1 Zeichen an Index 1
  const speakersVal = rawdata.charAt(1);
  console.log("[Echtzeit-Log] Extrahierter Lautsprecher-Wert:", speakersVal);

  // Aktualisiert das Status-Dropdown mit der ID "speakers"
  const speakersElem = document.getElementById("speakers");
  if (speakersElem) {
      speakersElem.value = speakersVal;
  }
  
  // 3. Lautstärke (EE) -> 2 Zeichen ab Index 2
  const volume = rawdata.slice(2, 4);
  const volElem = document.getElementById("volume");
  if (volElem) volElem.value = volume;
  
  // Slider im Control-Tab synchronisieren
  const sliderElem = document.getElementById("volumeSlider");
  if (sliderElem) sliderElem.value = parseInt(volume, 10) || 0;
  
  // --- DYNAMISCHER TUNER-BLOCK ---
  const stationElem = document.getElementById("tunerstation");
  const stationIdElem = document.getElementById("stationid");
  const freqElem = document.getElementById("frequency");

  if (b285source === "5" || b285source === "6") {
    // Wenn die Quelle Tuner ist, lesen wir die restlichen Zeichen ab Index 4 aus
    const tunerstation = rawdata.slice(4, 6);   // FF (2 Zeichen)
    const stationid = rawdata.slice(6, 10);     // GGGG (4 Zeichen)
    const frequency = rawdata.slice(10, 15);    // HHHHH (5 Zeichen) - z.B. "8850" oder "10730"

    if (stationElem) stationElem.value = tunerstation;
    if (stationIdElem) stationIdElem.value = stationid;
    
    if (freqElem) {
      // 1. String in echte Zahl umwandeln (z.B. 10730)
      const freqNum = parseFloat(frequency);
      
      if (!isNaN(freqNum)) {
        // 2. Durch 100 teilen (-> 107.3) und auf 2 Nachkommastellen zwingen (-> "107.30")
        // 3. Punkt durch Komma ersetzen und " MHz" anhängen
        const formattedFreq = (freqNum / 100).toFixed(2).replace('.', ',') + " MHz";
        freqElem.value = formattedFreq;
      } else {
        freqElem.value = frequency; // Fallback, falls keine Zahl extrahiert werden konnte
      }
    }
  } else {
    // Falls keine Tuner-Quelle aktiv ist, leeren wir die Status-Tunerfelder
    if (stationElem) stationElem.value = "--";
    if (stationIdElem) stationIdElem.value = "------";
    if (freqElem) freqElem.value = "------";
  }
  setB285Speakers(); 
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
    if (!response.ok) throw new Error(`HTTP-Fehler! Status: ${response.status}`);
    
    portconfigData = await response.json();

    syncSelectField();
    renderTable();
  } catch (error) {
    console.error("Fehler beim Laden der Portkonfiguration:", error);
  }
}

  function updateValue(index, field, value) {
    portconfigData[index][field] = value;
  }

async function saveData() {
  try {
    console.log("Sende Port-Konfiguration an den ESP...", portconfigData);
    
    const response = await fetch('/api/save-data', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json'
      },
      body: JSON.stringify(portconfigData) 
    });

    if (!response.ok) throw new Error(`Server antwortete mit Status: ${response.status}`);
    
    // Wir warten, bis der Server die Antwort ("JSON gespeichert") komplett gesendet hat
    const result = await response.json();
    console.log("Antwort vom ESP erhalten:", result);
    
    alert("Konfiguration erfolgreich im ESP gespeichert!");
    
    // Daten neu laden, um das UI zu aktualisieren
    await loadData(); 

    // HINWEIS: window.location.href = '/' wurde entfernt! 
    // Dadurch bleiben Sie im Tab und können sofort sehen, ob die Tabelle aktualisiert wurde.

  } catch (error) {
    console.error("Speicherfehler:", error);
    alert("Fehler beim Sichern der Daten im ESP-Speicher.");
  }
}

 async function loadConfig() {
    const tbody = document.getElementById('configTable');
    if (!tbody) {
        console.error("Meldung: Das Element 'configTable' wurde im HTML nicht gefunden!");
        return; 
    }
    
    try {
        // TIMING-SCHUTZ: Falls portconfigData noch leer ist, laden wir sie hier direkt sync nach
        if (!Array.isArray(portconfigData) || portconfigData.length === 0) {
            console.log("Meldung: portconfigData ist leer. Lade Port-Konfiguration direkt nach...");
            try {
                const portResponse = await fetch(`/portconfig.json?v=${Date.now()}`);
                if (portResponse.ok) {
                    portconfigData = await portResponse.json();
                }
            } catch (e) {
                console.error("Fehler beim automatischen Nachladen der Port-Konfiguration:", e);
            }
        }

        console.log("Meldung: Klick registriert. Starte Fetch-Anfrage an den ESP...");
        const response = await fetch('/api/config?_cb=' + Date.now());
        if (!response.ok) throw new Error('Laden fehlgeschlagen');
        
        configData = await response.json();
        console.log("Meldung: JSON erfolgreich empfangen. Anzahl Eintr\u00e4ge:", configData.length);
        
        tbody.innerHTML = '';
        const fragment = document.createDocumentFragment();
        
        const ignoriertBtnIDs = ["falseIR", "trueIR", "amppoweron", "tun20", "tun21", "tun22", "tun23", "tun24", "tun25", "tun26", "tun27", "tun28", "tun29", "none"];
        const ausnahmeCmdBtnIDs = ["b203reset"];

        // Robuste Prüfung auf das Wort 'receiver' in descr oder device
        let hatReceiverInPortconfig = false;
        if (Array.isArray(portconfigData) && portconfigData.length > 0) {
            hatReceiverInPortconfig = portconfigData.some(port => {
                if (!port) return false;
                const textMenge = `${port.descr || ''} ${port.device || ''}`.toLowerCase();
                return textMenge.includes('receiver');
            });
        }

        console.log("DEBUG: Wurde 'receiver' in der Portkonfiguration gefunden?", hatReceiverInPortconfig);

        configData.forEach((item, index) => {
            if (item.btnID && ignoriertBtnIDs.includes(item.btnID)) {
                return; 
            }
            
            const tr = document.createElement('tr');
            
            // HEX-Formatierung des IR-Codes
            let rawCode = item.irRecvCode;
            let displayCode = "0x0";
            if (typeof rawCode === 'number') {
                displayCode = '0x' + rawCode.toString(16).toUpperCase();
            } else if (typeof rawCode === 'string') {
                rawCode = rawCode.trim();
                if (rawCode.toLowerCase().startsWith('0x')) {
                    displayCode = '0x' + rawCode.substring(2).toUpperCase();
                } else {
                    let parsed = parseInt(rawCode, 10);
                    displayCode = isNaN(parsed) ? '0x0' : '0x' + parsed.toString(16).toUpperCase();
                }
            }

            // HEX-Formatierung für das bibusCmd (Fix für exakt 5 Zeichen nach 0x)
let rawBibus = item.bibusCmd;
let displayBibus = "";

if (rawBibus !== undefined && rawBibus !== null && rawBibus !== "") {
    if (typeof rawBibus === 'number') {
        // Zahl wird in Hex gewandelt und links mit Nullen auf 5 Stellen aufgefüllt
        displayBibus = '0x' + rawBibus.toString(16).toUpperCase().padStart(5, '0');
    } else if (typeof rawBibus === 'string') {
        rawBibus = rawBibus.trim();
        
        if (rawBibus.toLowerCase().startsWith('0x')) {
            // "0x" abschneiden, den Rest in Großbuchstaben wandeln und auf 5 Stellen auffüllen
            let hexPart = rawBibus.substring(2).toUpperCase();
            displayBibus = '0x' + hexPart.padStart(5, '0');
        } else if (/^[0-9a-fA-F]+$/.test(rawBibus)) {
            // Reiner Hex-String (z. B. "1A" oder "001A") -> direkt auf 5 Stellen bringen
            displayBibus = '0x' + rawBibus.toUpperCase().padStart(5, '0');
        } else {
            // Dezimalzahl als Text (z. B. "26") -> in Hex wandeln und auf 5 Stellen bringen
            let parsed = parseInt(rawBibus, 10);
            if (!isNaN(parsed)) {
                displayBibus = '0x' + parsed.toString(16).toUpperCase().padStart(5, '0');
            } else {
                displayBibus = rawBibus; // Fallback für Text
            }
        }
    }
}

            // Prüfen, ob der Befehl 0x40 entspricht
            let isCommand40 = false;
            if (item.command !== undefined && item.command !== null) {
                if (typeof item.command === 'number' && item.command === 64) {
                    isCommand40 = true;
                } else if (typeof item.command === 'string') {
                    let cmdStr = item.command.trim().toLowerCase();
                    if (cmdStr === '0x40' || cmdStr === '40' || parseInt(cmdStr, 16) === 64) {
                        isCommand40 = true;
                    }
                }
            }

            // Felder basierend auf Bedingungen vorbereiten
            let checkboxCmdHtml = "";
            const istAusnahmeBtn = item.btnID && ausnahmeCmdBtnIDs.includes(item.btnID);
            
            const btnIDLower = (item.btnID || "").toLowerCase();
            const istAmpOderTun = btnIDLower.startsWith("amp") || btnIDLower.startsWith("tun");

            // Ausblenden, wenn es ein amp/tun Button ist, aber KEIN receiver ermittelt wurde
            const hideAmpTun = HIDE_AMP_TUN_WITHOUT_RECEIVER && istAmpOderTun && !hatReceiverInPortconfig;

            // Zustand aus dem JSON für die gegenseitige Beeinflussung ermitteln
            const istSeriellImJsonAktiv = (item.cmdFlag === "1" || item.cmdFlag === 1 || item.cmdFlag === true);

            // LOGIK FÜR FLAG (SERIAL):
            if (hideAmpTun || istAusnahmeBtn) {
                checkboxCmdHtml = `<span class="placeholder-dash">-</span>`;
            } else if (item.serCmd !== null && item.serCmd !== undefined) {
                const isCmdChecked = istSeriellImJsonAktiv ? "checked" : "";
                checkboxCmdHtml = `<input type="checkbox" ${isCmdChecked} data-index="${index}" class="config-checkbox">`;
            } else {
                checkboxCmdHtml = `<span class="placeholder-dash">-</span>`;
            }

            // NEUE LOGIK FÜR BIBUS: Immer anzeigen, wenn es nicht Command 40 ist
            let checkboxBibusHtml = "";
            let bibusCmdInputHtml = "";

            if (!isCommand40) {
              const isBibusChecked = (item.isBibus === true && !istSeriellImJsonAktiv) ? "checked" : "";
              checkboxBibusHtml = `<input type="checkbox" ${isBibusChecked} data-index="${index}" class="bibus-checkbox">`;
              bibusCmdInputHtml = `<input type="text" class="bibusCmd-input" data-index="${index}" value="${displayBibus}">`;
            }
            
            // HTML-Struktur befüllen (Mit Breitenanpassung für die Eingabefelder)
            tr.innerHTML = `
                <td class="btnID-text">${item.btnID || '-'}</td>
                <td>
                    <input type="text" class="irRecvCode-input table-input-field" data-index="${index}" value="${displayCode}">
                </td>
                <td class="cell-center">
                    ${checkboxCmdHtml}
                </td>
                <td class="cell-center">
                    ${checkboxBibusHtml}
                </td>
                <td>
                    ${bibusCmdInputHtml ? bibusCmdInputHtml.replace('class="bibusCmd-input"', 'class="bibusCmd-input table-input-field"') : ''}
                </td>
            `;
            fragment.appendChild(tr);
        });

        tbody.appendChild(fragment);
        console.log("Meldung: Tabelle wurde erfolgreich im HTML sichtbar bef\u00fcllt!");

    } catch (error) {
        console.error("Meldung: Fehler im try-catch Block aufgetreten:", error);
        alert('Fehler beim Laden: ' + error.message);
    }
}

  async function saveConfig() {
    // ==================================================================
    // NEU: ABSICHERUNG
    // ==================================================================
    if (!Array.isArray(portconfigData) || portconfigData.length === 0) {
        console.log("Meldung: portconfigData war beim Speichern leer. Lade Port-Konfiguration nach...");
        try {
            const portResponse = await fetch(`/portconfig.json?v=${Date.now()}`);
            if (portResponse.ok) {
                portconfigData = await portResponse.json();
            }
        } catch (e) { 
            console.error("Fehler beim Laden im Speicher-Prozess:", e); 
        }
    }

    // ==================================================================
    // 1. IR-Codes verarbeiten
    const inputs = document.querySelectorAll('#configTable .irRecvCode-input');
    inputs.forEach(input => {
        const index = parseInt(input.getAttribute('data-index'), 10);
        const val = input.value.trim();
        
        if (!configData || configData[index] === undefined) return; 

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

    // ==================================================================
    // KORREKTUR: Identische, robuste Prüfung wie in der loadConfig-Funktion
    // ==================================================================
    let hatReceiverInPortconfig = false;
    if (Array.isArray(portconfigData) && portconfigData.length > 0) {
        hatReceiverInPortconfig = portconfigData.some(port => {
            if (!port) return false;
            const textMenge = `${port.descr || ''} ${port.device || ''}`.toLowerCase();
            return textMenge.includes('receiver');
        });
    }
    // ==================================================================

    // 2. ALLE Zeilen durchgehen für das cmdFlag und isBibus (Kombinierter Schritt)
    configData.forEach((item, index) => {
        if (!item || item.btnID === undefined) return;

        const ausnahmeCmdBtnIDs = ["b203reset", "b203enter"];
        const istAusnahmeBtn = ausnahmeCmdBtnIDs.includes(item.btnID);
        
        const btnIDLower = item.btnID.toLowerCase();
        const istAmpOderTun = btnIDLower.startsWith("amp") || btnIDLower.startsWith("tun");

        // ÄNDERUNG HIER: Entspricht Option 1 oder Option 2 aus Ihrem vorherigen Schritt.
        // Wenn HIDE_AMP_TUN_WITHOUT_RECEIVER in loadConfig deaktiviert ist, erzwingen wir hier false.
        const hideAmpTun = false; 

        if (hideAmpTun) {
            configData[index].cmdFlag = 0;
            configData[index].isBibus = false;
        } else if (istAusnahmeBtn) {
            configData[index].cmdFlag = 0;
            const bibusCheckbox = document.querySelector(`#configTable .bibus-checkbox[data-index="${index}"]`);
            if (bibusCheckbox) configData[index].isBibus = bibusCheckbox.checked;
        } else {
            // Sichtbare Standard-Zeilen: Werte direkt aus den Checkboxen auslesen
            const serialCheckbox = document.querySelector(`#configTable .config-checkbox[data-index="${index}"]`);
            if (serialCheckbox) {
                configData[index].cmdFlag = serialCheckbox.checked ? 1 : 0;
            }

            const bibusCheckbox = document.querySelector(`#configTable .bibus-checkbox[data-index="${index}"]`);
            if (bibusCheckbox) {
                configData[index].isBibus = bibusCheckbox.checked;
            }
        }
    });

    // ==================================================================
    // 4. Textfelder für bibusCmd verarbeiten (Fix für exakt 5 Zeichen nach 0x)
    // ==================================================================
    const bibusInputs = document.querySelectorAll('#configTable .bibusCmd-input');
    bibusInputs.forEach(input => {
        const index = parseInt(input.getAttribute('data-index'), 10);
        const val = input.value.trim();
        
        if (!configData || configData[index] === undefined) return; 

        if (val === "") {
            configData[index].bibusCmd = "";
            return;
        }

        let parsedNumber = NaN;
        if (val.toLowerCase().startsWith('0x')) {
            parsedNumber = parseInt(val, 16); 
        } else {
            parsedNumber = parseInt(val, 10); 
            // Wenn es keine Dezimalzahl ist, aber reines Hex (z. B. "1A"), als Hex parsen
            if (isNaN(parsedNumber) && /^[0-9a-fA-F]+$/.test(val)) {
                parsedNumber = parseInt(val, 16);
            }
        }

        if (isNaN(parsedNumber)) {
            configData[index].bibusCmd = val; 
        } else {
            // FIX: padStart(5, '0') erzwingt nun exakt 5 Stellen nach dem 0x im JSON
            configData[index].bibusCmd = "0x" + parsedNumber.toString(16).toUpperCase().padStart(5, '0');
        }
    });

    // 5. Daten an den ESP senden
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