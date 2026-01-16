# Huawei Solar Modbus Bridge (ESP32-S3-ETH)

### [ITA] Il Problema: Il maledetto Dongle LAN e i Timeout
Se sei qui, probabilmente hai un impianto Huawei con due inverter in **daisy chain** e stai cercando di leggere i dati tramite l'integrazione [Huawei Solar di wlcrs](https://github.com/wlcrs/huawei_solar). 

Sinceramente? Il dongle LAN ufficiale è un incubo per il Modbus. Nonostante avessi configurato tutto correttamente, le query fallivano di continuo. Ho provato ad alzare i timeout a dismisura (10s, 20s, 30s...), ma non è servito a nulla: connessione instabile, entità non disponibili in Home Assistant e buchi nei grafici di produzione. Il dongle semplicemente non regge il carico di query Modbus TCP su più dispositivi in cascata.

### La Soluzione: Due bridge dedicati
Invece di impazzire con i timeout, ho deciso di bypassare il dongle usando la connessione WiFi interna degli inverter (che è molto più reattiva). Ho acquistato due **Waveshare ESP32-S3-ETH** (modello con PoE per evitare alimentatori sparsi) e ho creato un bridge trasparente.

**Ecco come ho risolto:**
1.  **Hardware:** Due ESP32-S3-ETH alimentati via PoE.
2.  **Setup:** Un ESP si connette all'AP WiFi del Master, l'altro all'AP WiFi dello Slave.
3.  **Bridge:** Gli ESP espongono una porta 502 sulla mia LAN cablata e girano i pacchetti sulla porta 6607 del WiFi dell'inverter.
4.  **Home Assistant:** Ho aggiunto due istanze dell'integrazione Huawei Solar, puntando direttamente agli IP statici dei due ESP.

**Risultato:** Connessione solida come la roccia, refresh dei dati ogni **5 secondi** e zero interruzioni da settimane.

---

### [ENG] The Problem: The damn LAN Dongle and Timeouts
If you are reading this, you probably have a Huawei system with two inverters in **daisy chain** and you are trying to pull data using the [wlcrs/huawei_solar](https://github.com/wlcrs/huawei_solar) integration.

Frankly? The official LAN dongle is a nightmare for Modbus. Even with everything configured correctly, queries were constantly failing. I tried increasing the timeouts to ridiculous levels (10s, 20s, 30s...), but it didn't help: unstable connection, "unavailable" entities in Home Assistant, and gaps in production charts. The dongle simply cannot handle the Modbus TCP query load over multiple cascaded devices.

### The Solution: Two dedicated bridges
Instead of losing my mind over timeouts, I decided to bypass the dongle by using the inverters' internal WiFi connection (which is much more responsive). I bought two **Waveshare ESP32-S3-ETH** boards (PoE model to avoid messy power adapters) and created a transparent bridge.

**This is how I fixed it:**
1.  **Hardware:** Two ESP32-S3-ETH boards powered via PoE.
2.  **Setup:** One ESP connects to the Master's WiFi AP, the other to the Slave's WiFi AP.
3.  **Bridge:** The ESPs expose port 502 on my wired LAN and forward packets to port 6607 on the inverter's WiFi.
4.  **Home Assistant:** I added two instances of the Huawei Solar integration, pointing directly to the static IPs of the two ESPs.

**Result:** Rock-solid connection, **5-second** data refresh, and zero interruptions for weeks.

---

## Hardware
* **Board:** [Waveshare ESP32-S3-ETH](https://www.waveshare.com/wiki/ESP32-S3-ETH)
* **Power:** PoE (Power over Ethernet)
* **Protocol:** Modbus TCP Bridge (Port 502 LAN -> Port 6607 WiFi)
