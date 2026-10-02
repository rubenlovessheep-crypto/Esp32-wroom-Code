#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
 
// OLED scherm instellingen (128x64)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
 
// Definiëring voor de LED op pin 2
#define LED_PIN 2
 
// Wifi instellingen voor de captive portal
const char* apSSID = "NaSk-Omrekentool";
 
DNSServer dnsServer;
WebServer server(80);
const byte DNS_PORT = 53;
 
// Variabelen voor het OLED scherm en status
String richtingTekst = "Gereed";
String inputTekst = "Wacht op invoer";
String outputTekst = "---";
int lastStationCount = 0;
 
// Strakke, moderne Captive Portal HTML / CSS
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>NaSk Omrekentool • Pro</title>
<style>
    :root {
      --bg-gradient: linear-gradient(135deg, #0f172a 0%, #1e1b4b 100%);
      --card-bg: rgba(30, 41, 59, 0.7);
      --border-color: rgba(255, 255, 255, 0.1);
      --accent: #38bdf8;
      --accent-glow: rgba(56, 189, 248, 0.3);
      --text-main: #f8fafc;
      --text-sub: #94a3b8;
    }
    body {
      font-family: 'Segoe UI', system-ui, -apple-system, sans-serif;
      background: var(--bg-gradient);
      color: var(--text-main);
      margin: 0;
      padding: 20px;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      min-height: 95vh;
      box-sizing: border-box;
    }
    .container {
      width: 100%;
      max-width: 380px;
    }
    .card {
      background: var(--card-bg);
      backdrop-filter: blur(16px);
      -webkit-backdrop-filter: blur(16px);
      border: 1px solid var(--border-color);
      border-radius: 24px;
      padding: 30px;
      box-shadow: 0 20px 40px rgba(0, 0, 0, 0.5);
      text-align: center;
      box-sizing: border-box;
    }
    .badge {
      display: inline-block;
      background: rgba(56, 189, 248, 0.15);
      color: var(--accent);
      font-size: 0.75rem;
      font-weight: 700;
      text-transform: uppercase;
      letter-spacing: 1.5px;
      padding: 6px 12px;
      border-radius: 20px;
      margin-bottom: 15px;
      border: 1px solid rgba(56, 189, 248, 0.3);
    }
    h2 {
      color: var(--text-main);
      font-size: 1.5rem;
      margin: 0 0 8px 0;
      font-weight: 700;
    }
    p {
      font-size: 0.9rem;
      color: var(--text-sub);
      margin-bottom: 25px;
      line-height: 1.4;
    }
    .input-group {
      margin-bottom: 18px;
      text-align: left;
    }
    label {
      display: block;
      font-size: 0.8rem;
      color: var(--text-sub);
      margin-bottom: 6px;
      font-weight: 600;
      text-transform: uppercase;
      letter-spacing: 0.5px;
    }
    select, input[type="number"] {
      width: 100%;
      padding: 14px;
      border-radius: 14px;
      border: 1px solid rgba(255, 255, 255, 0.15);
      background: rgba(15, 23, 42, 0.6);
      color: var(--text-main);
      box-sizing: border-box;
      font-size: 1rem;
      outline: none;
    }
    select:focus, input[type="number"]:focus {
      border-color: var(--accent);
      box-shadow: 0 0 15px var(--accent-glow);
    }
    button {
      width: 100%;
      background: linear-gradient(135deg, #3b82f6 0%, #2563eb 100%);
      color: white;
      border: none;
      padding: 15px;
      border-radius: 14px;
      font-weight: 700;
      font-size: 1rem;
      cursor: pointer;
      box-shadow: 0 8px 20px rgba(37, 99, 235, 0.4);
      margin-top: 10px;
    }
    button:active {
      transform: translateY(1px);
    }
    .footer {
      margin-top: 20px;
      font-size: 0.75rem;
      color: #64748b;
      text-align: center;
    }
</style>
</head>
<body>
<div class="container">
<div class="card">
<div class="badge">Natuurkunde Tool</div>
<h2>Omrekentool</h2>
<p>Bereken direct en stuur de data naar het OLED-scherm.</p>
<form action="/bereken" method="GET">
<div class="input-group">
<label>Richting</label>
<select name="richting">
<option value="ms_naar_kmh">m/s &rarr; km/h (&times; 3.6)</option>
<option value="kmh_naar_ms">km/h &rarr; m/s (&divide; 3.6)</option>
</select>
</div>
 
        <div class="input-group">
<label>Snelheid Waarde</label>
<input type="number" step="0.1" name="waarde" placeholder="bijv. 25" required>
</div>
 
        <button type="submit">Bereken & Toon</button>
</form>
</div>
<div class="footer">ESP32 DevKit Build + Live Connect</div>
</div>
</body>
</html>
)rawliteral";
 
const char SUCCESS_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="nl">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Resultaat • NaSk Pro</title>
<style>
    body {
      font-family: 'Segoe UI', system-ui, sans-serif;
      background: linear-gradient(135deg, #0f172a 0%, #1e1b4b 100%);
      color: #f8fafc;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 95vh;
      margin: 0;
      padding: 20px;
      box-sizing: border-box;
    }
    .card {
      background: rgba(30, 41, 59, 0.7);
      backdrop-filter: blur(16px);
      border: 1px solid rgba(255, 255, 255, 0.1);
      padding: 35px;
      border-radius: 24px;
      box-shadow: 0 20px 40px rgba(0,0,0,0.5);
      max-width: 340px;
      width: 100%;
      text-align: center;
    }
    h2 { color: #38bdf8; margin-top: 0; font-size: 1.3rem; }
    .res-box {
      background: rgba(16, 185, 129, 0.1);
      border: 1px solid rgba(16, 185, 129, 0.3);
      color: #34d399;
      padding: 15px;
      border-radius: 14px;
      font-size: 1.3rem;
      font-weight: 700;
      margin: 20px 0;
      word-break: break-all;
    }
    p { font-size: 0.85rem; color: #94a3b8; }
    a {
      display: block;
      background: #3b82f6;
      color: white;
      text-decoration: none;
      padding: 14px;
      border-radius: 14px;
      font-weight: 700;
      margin-top: 20px;
    }
</style>
</head>
<body>
<div class="card">
<h2>Succesvol Verstuurd!</h2>
<p>De berekening staat op het OLED-scherm:</p>
<div class="res-box" id="resultaatVeld">Laden...</div>
<a href="/">&larr; Nieuwe berekening</a>
</div>
</body>
</html>
)rawliteral";
 
// Langere, soepele laadanimatie met strakke coördinaten en knipperende LED
void showLoadingAnimation() {
  for (int i = 0; i <= 100; i += 5) { // Meer stappen (per 5%) voor langere duur
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    // Tekst iets hoger en netjes gecentreerd
    display.setCursor(16, 2);
    display.print("= Data Ontvangen =");
    display.drawLine(0, 12, 127, 12, SSD1306_WHITE); // Lijn strak op y = 12
 
    display.setCursor(22, 18);
    display.print("Berekenen...");
 
    // Laadbalk tekenen
    display.drawRect(14, 32, 100, 14, SSD1306_WHITE);
    int fillWidth = map(i, 0, 100, 0, 96);
    display.fillRect(16, 34, fillWidth, 10, SSD1306_WHITE);
 
    display.setCursor(38, 50);
    display.print(i);
    display.print("% voltooid");
 
    display.display();
 
    // Ritmisch knipperen van de LED tijdens het laden
    digitalWrite(LED_PIN, (i / 10) % 2 == 0 ? HIGH : LOW);
    delay(55); // Langere vertraging per stap voor een mooi tempo
  }
  digitalWrite(LED_PIN, LOW); // LED uit aan het einde
}
 
void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  // Header netjes uitgelijnd
  display.setCursor(22, 2);
  display.print("= NaSk PRO =");
  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);
 
  display.setCursor(0, 18);
  display.print("Modus: ");
  display.print(richtingTekst);
 
  display.setCursor(0, 32);
  display.print("In:  ");
  display.print(inputTekst);
 
  display.setCursor(0, 48);
  display.print("Uit: ");
  display.print(outputTekst);
 
  display.display();
}
 
void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}
 
void handleBereken() {
  if (server.hasArg("waarde") && server.hasArg("richting")) {
    showLoadingAnimation();
 
    float waarde = server.arg("waarde").toFloat();
    String richting = server.arg("richting");
    float resultaat = 0.0;
    String resString = "";
 
    if (richting == "ms_naar_kmh") {
      resultaat = waarde * 3.6;
      richtingTekst = "ms -> kmh";
      inputTekst = String(waarde, 1) + " m/s";
      outputTekst = String(resultaat, 1) + " km/h";
      resString = String(waarde, 1) + " m/s = " + String(resultaat, 1) + " km/h";
    } else {
      resultaat = waarde / 3.6;
      richtingTekst = "kmh -> ms";
      inputTekst = String(waarde, 1) + " km/h";
      outputTekst = String(resultaat, 1) + " m/s";
      resString = String(waarde, 1) + " km/h = " + String(resultaat, 1) + " m/s";
    }
 
    updateOLED();
 
    String customSuccess = String(SUCCESS_page);
    customSuccess.replace("Laden...", resString);
    server.send(200, "text/html", customSuccess);
  } else {
    server.send(200, "text/plain", "Ongeldige invoer");
  }
}
 
void setup() {
  Serial.begin(115200);
 
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
 
  Wire.begin(21, 22);
 
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 mislukt!"));
    while(1);
  }
  // Uitgebreid, langer bootscherm
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(16, 15);
  display.print("NaSk Tool Pro");
  display.setCursor(10, 30);
  display.print("Systeem starten...");
  display.drawRect(14, 45, 100, 10, SSD1306_WHITE);
  display.fillRect(16, 47, 60, 6, SSD1306_WHITE);
  display.display();
  delay(2500); // Langer boot-moment
 
  WiFi.softAP(apSSID);
  dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());
 
  server.on("/", handleRoot);
  server.on("/bereken", handleBereken);
  server.onNotFound(handleRoot);
  server.begin();
 
  display.clearDisplay();
  display.setCursor(0, 2);
  display.print("AP: NaSk-Omrekentool");
  display.drawLine(0, 12, 127, 12, SSD1306_WHITE);
  display.setCursor(0, 18);
  display.print("1. Verbind wifi");
  display.setCursor(0, 32);
  display.print("2. Open browser");
  display.setCursor(0, 48);
  display.print("3. Wacht op apparaat");
  display.display();
}
 
void loop() {
  dnsServer.processNextRequest();
  server.handleClient();
 
  // Controleer of er een apparaat verbindt of verbinding verbreekt
  int currentStations = WiFi.softAPgetStationNum();
  if (currentStations != lastStationCount) {
    lastStationCount = currentStations;
 
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(14, 2);
    display.print("= WiFi Status =");
    display.drawLine(0, 12, 127, 12, SSD1306_WHITE);
 
    display.setCursor(0, 22);
    if (currentStations > 0) {
      display.print("Apparaat Verbonden!");
      display.setCursor(0, 38);
      display.print("Actief: ");
      display.print(currentStations);
      display.print(" telefoon");
      digitalWrite(LED_PIN, HIGH); // LED blijft branden zolang er iemand verbonden is
    } else {
      display.print("Geen apparaten");
      display.setCursor(0, 38);
      display.print("Wacht op verbinding");
      digitalWrite(LED_PIN, LOW); // LED uit als er niemand verbonden is
    }
    display.display();
    delay(2000); // Laat de statusmelding 2 seconden zien
    updateOLED(); // Schakel daarna terug naar het normale infoscherm (of het laatste resultaat)
  }
}