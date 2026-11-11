# Tabella di marcia — MouseTester

## Fase 0 — Impostazione
- [x] Scheletro del progetto: cattura Raw Input, coda lock-free, analizzatore, interfaccia ImGui
- [x] README, CLAUDE.md, .gitignore
- [x] Repository git e primo commit
- [x] Repository privato su GitHub (`lcrd42/MouseTester`)

## Fase 1 — Prima compilazione
- [x] Configurare e compilare con CMake e Visual Studio (x64-Release)
- [x] Correggere gli errori di compilazione dello scheletro (nessuno: compila pulito con /W3)
- [x] La finestra si apre, ImGui disegna, il mouse vettoriale si vede
- [x] Il thread di cattura riceve WM_INPUT e la coda si svuota a ogni frame (~8000 pacchetti/s ricevuti)
- [x] I tasti del mouse disegnato si illuminano alla pressione

## Fase 2 — Polling rate affidabile
- [x] Rate calcolato dalla media degli intervalli e non dalla mediana (a 8000 Hz segnava ~11000 Hz)
- [x] Scale numeriche in µs e linea di riferimento sui grafici del polling
- [ ] Verifica a 125, 500 e 1000 Hz: rate, jitter e istogramma coerenti
- [x] Verifica a 4000 e 8000 Hz: nessun pacchetto perso dal thread di cattura (verificato dall'utente)
- [x] Interfaccia fluida a 8000 Hz: ciclo dei messaggi limitato a 2 ms (prima fino a 475 ms per frame);
      swap chain "blt", perché "flip" faceva scendere lo schermo a ~65 Hz a mouse fermo
- [x] Misura attiva da 1,5 count per pacchetto, con stato "in misura / in pausa" e velocità attuale
- [x] Indicatore dei tempi di frame in alto a destra
- [x] Valutare GetRawInputBuffer: non serve, a 8000 Hz la coda dei messaggi regge senza perdite
- [x] CPU del thread di cattura misurata nell'app con QueryThreadCycleTime e mostrata in alto a destra:
      ~18% di un core a ~7000 pacchetti/s simulati, 24 µs per pacchetto (meno dell'1% della CPU totale)
- [x] Ritardo tra arrivo e timestamp: non misurabile via software (MSG.time ha risoluzione di millisecondi);
      il timestamp QPC viene preso appena il messaggio esce dalla coda
- [x] Filtro per dispositivo: menu in alto a sinistra, con il numero di pacchetti per dispositivo
- [ ] Confronto dei risultati con un altro tester noto

## Fase 3 — Sensore e DPI
- [x] Continuità valutata sui count di ogni pacchetto rispetto ai 4 vicini, non sui tempi di arrivo
- [x] Tracciato con zoom, spostamento, punti dei singoli pacchetti e anomalie segnate
- [ ] Taratura delle soglie per pacchetti mancanti, picchi, cali e inversioni
- [x] Angle snapping: retta dell'ultimo tratto (angolo, scostamento RMS), guide a 0/45/90/135°,
      avviso se il tratto è lungo, quasi perfetto e su una guida (verificato con tratti sintetici)
- [x] Spiegazioni al passaggio del mouse sui contatori del sensore
- [x] Misura DPI guidata: distanza nota (cm o pollici), passate col tasto sinistro premuto sull'area di misura,
      media, dispersione, min/max, angolo di ogni passata
- [ ] Verifica della misura DPI ai valori impostati nel software del mouse

## Fase 4 — Click e rotella
- [ ] Taratura della soglia di chatter (default 30 ms) con prove reali
- [ ] Rotella: passi persi, inversioni, tilt
- [ ] Tasti laterali (4 e 5) disegnati e conteggiati

## Fase 5 — Rifinitura
- [ ] Tema e disegno del mouse curati
- [ ] Scala corretta su monitor ad alta densità (DPI awareness)
- [x] Esportare i risultati: report di testo di tutte le schede e CSV degli intervalli del polling
- [x] Icona dell'eseguibile e della finestra (`res/app.ico`, rigenerabile con `tools/make_icon.ps1`)
- [x] Informazioni di versione dell'eseguibile (0.1.0, in `res/resource.h`)
- [x] Build Release distribuibile: eseguibile unico con CRT statica, dipende solo da DLL di sistema
- [x] Versione 0.1.0 rilasciata: tag `v0.1.0` e `dist/MouseTester-0.1.0.zip` (exe + `dist/README.txt` per gli utenti)

## Versione 0.2.0
- [x] Barra di scorrimento della finestra di destra riparata (l'indicatore CPU/frame spostava il cursore di disegno)
- [x] Menu della lingua: inglese, italiano, russo, spagnolo, francese, tedesco, portoghese, cinese semplificato;
      scelta salvata nel registro (HKCU\Software\MouseTester), al primo avvio segue la lingua di Windows
- [x] Font: Segoe UI con cirillico, Microsoft YaHei unito per il cinese, solo i caratteri usati
- [ ] Hindi, arabo, bengalese: richiedono la composizione dei caratteri (e l'arabo la scrittura da destra), che Dear ImGui non supporta
- [ ] Traduzioni da far rileggere a madrelingua (scritte senza revisione)

## Pubblicazione
- [x] Open source con licenza GPL-3.0-or-later (`LICENSE`, avvisi SPDX nei sorgenti, `THIRD_PARTY.txt` per Dear ImGui)
- [x] Email dei commit sostituita con quella anonima di GitHub in tutta la storia, prima di pubblicare
- [x] Repository pubblico, README per GitHub con screenshot, release v0.2.0 con lo zip
- [x] Screenshot reali nel README (polling a 8000 Hz, sensore, click e rotella)
- [ ] Post su r/MouseReview e nei Discord dedicati
- [ ] Firma digitale dell'exe (es. Azure Trusted Signing) per evitare l'avviso di SmartScreen
- [ ] Pacchetto winget
