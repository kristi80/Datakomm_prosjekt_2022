# datakomm_gutta
Her finner man alle filer nødvendig for å kunne gjennomføre prosjektet.
## For de som er ukjent med PlatformIO
Hovedkoden ligger under "src" og heter "main.cpp"

## Node-RED
For å kjøre node red koden, må man laste ned Node-red på en maskin.
Deretter går man til øverste "burgermeny" og velger "import" og filtype .json. Du limer her inn koden 
fra GitHub filen "node_red_flow" i "Datakomm_prosjekt_2022". Når dette er lastet ned finner man
en kommentar i Dashboard flow, som gir videre info.

## For å kjøre alt
Last ned kode fra "testpanel" på en ESP32 og koble opp komponentene fra rapporten.
Deretter kjør en MQTT server, dette kan gjøres fra VS-code ved hjelp av "mosquitto" folder (her finnes flere kommentarer).
Start en Node-RED server som kobles opp til MQTT server.
Nå skal komponenter og servere være koblet opp og snakke sammen.
