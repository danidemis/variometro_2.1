#include "WebHandler.h"
#include <SD.h> // FONDAMENTALE: Ci serve per leggere i file direttamente!

AsyncWebServer server(80);

const char WebHandler::index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta charset="UTF-8"> <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>SkyVario Config</title>
  <style>
    body { font-family: Arial, sans-serif; margin:0px; padding:20px; background-color: #f4f4f9; color: #333; }
    h2 { color: #0056b3; text-align: center; }
    form { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); margin-top: 20px;}
    
    .tab { overflow: hidden; border-bottom: 2px solid #ccc; display: flex; }
    .tab button { background-color: inherit; border: none; outline: none; cursor: pointer; padding: 14px 16px; transition: 0.3s; font-size: 16px; font-weight: bold; flex-grow: 1; color: #555; }
    .tab button:hover { background-color: #e9e9e9; }
    .tab button.active { background-color: #0056b3; color: white; border-radius: 8px 8px 0 0; }
    
    .tabcontent { display: none; padding: 15px 0; animation: fadeEffect 0.5s; }
    @keyframes fadeEffect { from {opacity: 0;} to {opacity: 1;} }

    label { font-weight: bold; display: block; margin-top: 15px; font-size: 14px; }
    input[type=text], input[type=password], input[type=number] { width: 100%; padding: 10px; margin-top: 5px; border: 1px solid #ccc; border-radius: 4px; box-sizing: border-box; }
    
    input[type=range] { width: 100%; margin-top: 10px; cursor: pointer; }
    .vol-display { color: #0056b3; font-weight: bold; }

    input[type=submit] { background-color: #28a745; color: white; border: none; padding: 15px; margin-top: 25px; width: 100%; cursor: pointer; font-size: 16px; font-weight: bold; border-radius: 4px; }
    input[type=submit]:hover { background-color: #218838; }
    .note { font-size: 12px; color: #666; margin-top: 2px; display: block; }
    
    .track-item { display:flex; justify-content:space-between; align-items:center; background:white; padding:12px; border-radius:5px; box-shadow:0 1px 3px rgba(0,0,0,0.1); margin-bottom:10px; }
    .btn-down { background:#28a745; color:white; padding:6px 12px; text-decoration:none; border-radius:4px; margin-right:5px; font-size:14px; font-weight:bold;}
    .btn-del { background:#dc3545; color:white; padding:6px 12px; border:none; border-radius:4px; cursor:pointer; font-size:14px; font-weight:bold;}
  </style>
</head>
<body>
  <h2>Impostazioni SkyVario</h2>
  
  <div class="tab">
    <button class="tablinks active" onclick="openTab(event, 'Anagrafica')">Rete</button>
    <button class="tablinks" onclick="openTab(event, 'Vario')">Vario</button>
    <button class="tablinks" onclick="openTab(event, 'Tracce')">Tracce IGC</button>
    <button class="tablinks" onclick="openTab(event, 'Diag')">Sensori</button>
  </div>

  <form action="/save" method="POST" id="configForm">
    <div id="Anagrafica" class="tabcontent" style="display:block;">
      <label>Nome Pilota</label>
      <input type="text" name="pilotName" value="%PILOT_NAME%">
      <label>Tipo Vela</label>
      <input type="text" name="gliderType" value="%GLIDER_TYPE%">
      <label>Nome Bluetooth (BLE)</label>
      <input type="text" name="bleName" value="%BLE_NAME%">
      <label>WiFi SSID (Rete Config)</label>
      <input type="text" name="wifiSSID" value="%WIFI_SSID%">
      <label>WiFi Password</label>
      <input type="password" name="wifiPass" value="%WIFI_PASS%">
      <input type="submit" value="Salva Impostazioni">
    </div>

    <div id="Vario" class="tabcontent">
      <label>Volume: <span id="volVal" class="vol-display">%VARIO_VOL%</span>%P%</label>
      <input type="range" name="varioVolume" min="0" max="100" value="%VARIO_VOL%" oninput="document.getElementById('volVal').innerText=this.value" onchange="testAudio(this.value)">
      <span class="note">Trascina e rilascia per un test audio.</span>
      <label>Sensibilità (m/s)</label>
      <input type="number" step="0.05" name="varioSensitivity" value="%VARIO_SENS%">
      <label style="display:flex; align-items:center; margin-top:25px; cursor:pointer;">
        <input type="checkbox" name="autoLog" value="1" style="width:20px; height:20px; margin-right:10px;" %AUTO_LOG_CHECKED%> Avvia registrazione IGC in automatico
      </label>
      <label style="display:flex; align-items:center; margin-top:10px; cursor:pointer;">
        <input type="checkbox" name="autoElevation" value="1" style="width:20px; height:20px; margin-right:10px;" onchange="toggleManualAlt(this)" %AUTO_ELEV_CHECKED%> Calibrazione Altitudine Auto (GPS)
      </label>
      <div id="manualAltDiv" style="display:%MANUAL_ALT_DISPLAY%; margin-top:10px;">
        <label>Altitudine di Decollo / Attuale (m)</label>
        <input type="number" name="manualElevation" value="%MANUAL_ELEV%">
      </div>
      <input type="submit" value="Salva Impostazioni">
    </div>
  </form>

  <div id="Tracce" class="tabcontent">
    <div style="display:flex; justify-content:space-between; align-items:center; margin-bottom:15px;">
      <h3 style="margin:0; color:#0056b3;">Archivio Voli</h3>
      <button onclick="loadTracks()" style="padding:8px 15px; background:#0056b3; color:white; border:none; border-radius:4px; cursor:pointer;">🔄 Aggiorna</button>
    </div>
    <div id="trackList" style="background:#eee; padding:15px; border-radius:5px; max-height:400px; overflow-y:auto;">
       Caricamento...
    </div>
  </div>

  <div id="Diag" class="tabcontent">
    <h3 style="color:#0056b3">Stato Sensori in Tempo Reale</h3>
    <div style="background:#eee; padding:15px; border-radius:5px; line-height:1.6;">
      <p>📡 <b>GPS:</b> <span id="gps-sat">0</span> Sat | HDOP: <span id="gps-hdop">-</span></p>
      <p>📍 <b>Pos:</b> <span id="gps-lat">-</span>, <span id="gps-lng">-</span></p>
      <p>🏔️ <b>Alt GPS:</b> <span id="gps-alt">-</span> m</p>
      <p>🧭 <b>Bussola:</b> <span id="comp-head">-</span>°</p>
      <p>🔋 <b>Batteria:</b> <span id="batt-volt">-</span>%</p>
    </div>
  </div>

  <script>
    function openTab(evt, tabName) {
      let i, tabcontent, tablinks;
      tabcontent = document.getElementsByClassName("tabcontent");
      for (i = 0; i < tabcontent.length; i++) tabcontent[i].style.display = "none";
      tablinks = document.getElementsByClassName("tablinks");
      for (i = 0; i < tablinks.length; i++) tablinks[i].className = tablinks[i].className.replace(" active", "");
      document.getElementById(tabName).style.display = "block";
      evt.currentTarget.className += " active";
      
      document.getElementById('configForm').style.display = (tabName === 'Anagrafica' || tabName === 'Vario') ? 'block' : 'none';
      if (tabName === 'Tracce') loadTracks(); // Carica in automatico quando apri il tab!
    }

    function testAudio(val) { fetch('/test_audio?vol=' + val); }
    function toggleManualAlt(cb) { document.getElementById('manualAltDiv').style.display = cb.checked ? 'none' : 'block'; }

    // --- FUNZIONI PER LE TRACCE IGC ---
    function loadTracks() {
      document.getElementById('trackList').innerHTML = "Lettura MicroSD in corso...";
      fetch('/api/tracks')
        .then(res => res.json())
        .then(data => {
          if(data.length === 0) {
            document.getElementById('trackList').innerHTML = "Nessuna traccia trovata sulla MicroSD.";
            return;
          }
          let html = "<div style='display:flex; flex-direction:column-reverse;'>"; // Mostra i più recenti in alto
          data.forEach(f => {
            html += `<div class='track-item'>
              <span><b>${f.name.replace('/','')}</b> <br><small>${(f.size/1024).toFixed(1)} KB</small></span>
              <div style="white-space:nowrap;">
                <a href='/download?file=${encodeURIComponent(f.name)}' class='btn-down'>⬇️</a>
                <button onclick="deleteTrack('${f.name}')" class='btn-del'>🗑️</button>
              </div>
            </div>`;
          });
          html += "</div>";
          document.getElementById('trackList').innerHTML = html;
        })
        .catch(e => { document.getElementById('trackList').innerHTML = "Errore di connessione."; });
    }

    function deleteTrack(filename) {
      if(confirm("Vuoi davvero eliminare la traccia " + filename + "?\nQuesta operazione è irreversibile!")) {
        fetch('/api/delete?file=' + encodeURIComponent(filename), {method: 'DELETE'})
          .then(res => res.text())
          .then(msg => { alert(msg); loadTracks(); });
      }
    }

    // --- AGGIORNAMENTO SENSORI ---
    setInterval(function() {
      fetch('/api/diag').then(res => res.json()).then(data => {
        document.getElementById('gps-sat').innerText = data.sat;
        document.getElementById('gps-hdop').innerText = data.hdop;
        document.getElementById('gps-lat').innerText = data.lat; 
        document.getElementById('gps-lng').innerText = data.lng; 
        document.getElementById('gps-alt').innerText = data.alt; 
        document.getElementById('comp-head').innerText = data.head;
        document.getElementById('batt-volt').innerText = data.batt;
      }).catch(e => {});
    }, 1000);
  </script>
</body>
</html>
)rawliteral";

void WebHandler::begin(StorageHandler* storage, AudioHandler* audio, GpsHandler* gps, CompassHandler* compass, BatteryHandler* battery) {
    _storage = storage; _audio = audio; _gps = gps; _compass = compass; _battery = battery;

    // Sottorete custom per evitare conflitti con la rete di casa
    IPAddress local_IP(192, 168, 203, 1);
    IPAddress gateway(192, 168, 203, 1);
    IPAddress subnet(255, 255, 255, 0);
    WiFi.softAPConfig(local_IP, gateway, subnet);
    
    WiFi.softAP(_storage->config.wifiSSID.c_str(), _storage->config.wifiPass.c_str());
    Serial.print("IP Web Server: "); Serial.println(WiFi.softAPIP());

    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(204); });

    server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request){
        String html = String(index_html);
        html.replace("%P%", "%"); 
        html.replace("%PILOT_NAME%", _storage->config.pilotName);
        html.replace("%GLIDER_TYPE%", _storage->config.gliderType);
        html.replace("%WIFI_SSID%", _storage->config.wifiSSID);
        html.replace("%WIFI_PASS%", _storage->config.wifiPass);
        html.replace("%BLE_NAME%", _storage->config.bleName);
        html.replace("%VARIO_VOL%", String(_storage->config.varioVolume));
        html.replace("%VARIO_SENS%", String(_storage->config.varioSensitivity, 2));
        html.replace("%AUTO_LOG_CHECKED%", _storage->config.autoLog ? "checked" : "");
        html.replace("%AUTO_ELEV_CHECKED%", _storage->config.autoElevation ? "checked" : "");
        html.replace("%MANUAL_ALT_DISPLAY%", _storage->config.autoElevation ? "none" : "block");
        html.replace("%MANUAL_ELEV%", String(_storage->config.manualElevation));
        request->send(200, "text/html; charset=utf-8", html);
    });

    server.on("/save", HTTP_POST, [this](AsyncWebServerRequest *request){
        if(request->hasParam("pilotName", true)) _storage->config.pilotName = request->getParam("pilotName", true)->value();
        if(request->hasParam("gliderType", true)) _storage->config.gliderType = request->getParam("gliderType", true)->value();
        if(request->hasParam("wifiSSID", true)) _storage->config.wifiSSID = request->getParam("wifiSSID", true)->value();
        if(request->hasParam("wifiPass", true)) _storage->config.wifiPass = request->getParam("wifiPass", true)->value();
        if(request->hasParam("bleName", true)) _storage->config.bleName = request->getParam("bleName", true)->value();
        if(request->hasParam("varioVolume", true)) _storage->config.varioVolume = request->getParam("varioVolume", true)->value().toInt();
        if(request->hasParam("varioSensitivity", true)) _storage->config.varioSensitivity = request->getParam("varioSensitivity", true)->value().toFloat();
        _storage->config.autoLog = request->hasParam("autoLog", true);
        _storage->config.autoElevation = request->hasParam("autoElevation", true);
        if(request->hasParam("manualElevation", true)) _storage->config.manualElevation = request->getParam("manualElevation", true)->value().toInt();
        
        _storage->saveConfig();
        request->send(200, "text/html; charset=utf-8" ,"<html><body style='font-family:Arial;text-align:center;padding:50px;'><h2>Impostazioni Salvate!</h2><p>Riavvia il variometro per applicare.</p><br><a href='/'>Torna indietro</a></body></html>");
    });

    server.on("/api/diag", HTTP_GET, [this](AsyncWebServerRequest *request){
        String json = "{";
        json += "\"sat\":" + String(_gps->getSatellites()) + ",";
        json += "\"hdop\":\"" + String(_gps->getHdop(), 1) + "\",";
        json += "\"alt\":\"" + String(_gps->getAlt(), 1) + "\",";
        json += "\"lat\":\"" + String(_gps->getLat(), 6) + "\",";
        json += "\"lng\":\"" + String(_gps->getLng(), 6) + "\",";
        json += "\"head\":" + String(_compass->getHeading()) + ",";
        json += "\"batt\":" + String(_battery->getPercentage());
        json += "}";
        request->send(200, "application/json; charset=utf-8", json);
    });

    server.on("/test_audio", HTTP_GET, [this](AsyncWebServerRequest *request){
        if(request->hasParam("vol")) {
            int vol = request->getParam("vol")->value().toInt();
            if (_audio) _audio->triggerTestBeep(vol);
        }
        request->send(200, "text/plain", "OK");
    });

    // --- LE TRE NUOVE API PER LA MICROSD ---

    // 1. Legge la MicroSD e crea una lista di tutti i file .igc
    server.on("/api/tracks", HTTP_GET, [](AsyncWebServerRequest *request){
        String json = "[";
        File root = SD.open("/");
        if(!root){
            request->send(500, "application/json", "[]");
            return;
        }
        File file = root.openNextFile();
        bool first = true;
        while(file){
            if(!file.isDirectory()){
                String fileName = String(file.name());
                if(!fileName.startsWith("/")) fileName = "/" + fileName; // Sicurezza per la sintassi

                // Controlla se finisce con .igc (Maiuscolo o Minuscolo)
                if(fileName.endsWith(".igc") || fileName.endsWith(".IGC")){
                    if(!first) json += ",";
                    json += "{\"name\":\"" + fileName + "\",\"size\":" + String(file.size()) + "}";
                    first = false;
                }
            }
            file = root.openNextFile();
        }
        json += "]";
        request->send(200, "application/json", json);
    });

    // 2. Forza il Download del file selezionato
    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
        if(request->hasParam("file")){
            String fileName = request->getParam("file")->value();
            if(SD.exists(fileName)){
                // L'ultimo 'true' forza il browser a scaricare il file anziché cercare di aprirlo
                request->send(SD, fileName, "application/octet-stream", true); 
            } else {
                request->send(404, "text/plain", "File non trovato.");
            }
        } else {
            request->send(400, "text/plain", "Nome file mancante.");
        }
    });

    // 3. Elimina il file dalla MicroSD
    server.on("/api/delete", HTTP_DELETE, [](AsyncWebServerRequest *request){
        if(request->hasParam("file")){
            String fileName = request->getParam("file")->value();
            if(SD.exists(fileName)){
                SD.remove(fileName);
                request->send(200, "text/plain", "Traccia eliminata con successo!");
            } else {
                request->send(404, "text/plain", "File non trovato.");
            }
        } else {
            request->send(400, "text/plain", "Nome file mancante.");
        }
    });

    server.begin();
}