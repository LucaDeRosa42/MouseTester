# Tabella di marcia — MouseTester

## Fase 0 — Impostazione
- [x] Scheletro del progetto: cattura Raw Input, coda lock-free, analizzatore, interfaccia ImGui
- [x] README, CLAUDE.md, .gitignore
- [x] Repository git e primo commit
- [x] Repository privato su GitHub (`lcrd42/MouseTester`)

## Fase 1 — Prima compilazione
- [x] Configurare e compilare con CMake e Visual Studio (x64-Release)
- [x] Correggere gli errori di compilazione dello scheletro (nessuno: compila pulito con /W3)
- [ ] La finestra si apre, ImGui disegna, il mouse vettoriale si vede
- [ ] Il thread di cattura riceve WM_INPUT e la coda si svuota a ogni frame
- [ ] I tasti del mouse disegnato si illuminano alla pressione

## Fase 2 — Polling rate affidabile
- [x] Rate calcolato dalla media degli intervalli e non dalla mediana (a 8000 Hz segnava ~11000 Hz)
- [x] Scale numeriche in µs e linea di riferimento sui grafici del polling
- [ ] Verifica a 125, 500 e 1000 Hz: rate, jitter e istogramma coerenti
- [ ] Verifica a 4000 e 8000 Hz: nessun pacchetto perso dal thread di cattura
- [ ] Valutare GetRawInputBuffer al posto di un GetRawInputData per messaggio, se a 8000 Hz la coda dei messaggi non regge
- [ ] Misurare quanto costa il thread di cattura (CPU) e il ritardo tra arrivo e timestamp
- [ ] Filtro per dispositivo: scegliere quale mouse misurare quando ce ne sono più di uno
- [ ] Confronto dei risultati con un altro tester noto

## Fase 3 — Sensore e DPI
- [x] Continuità valutata sui count di ogni pacchetto rispetto ai 4 vicini, non sui tempi di arrivo
- [x] Tracciato con zoom, spostamento, punti dei singoli pacchetti e anomalie segnate
- [ ] Taratura delle soglie per pacchetti mancanti, picchi, cali e inversioni
- [ ] Tracciato per l'angle snapping: linee orizzontali e diagonali leggibili
- [ ] Misura DPI guidata: distanza nota (righello o tappetino), più passate, media e scarto
- [ ] Verifica della misura DPI ai valori impostati nel software del mouse

## Fase 4 — Click e rotella
- [ ] Taratura della soglia di chatter (default 30 ms) con prove reali
- [ ] Rotella: passi persi, inversioni, tilt
- [ ] Tasti laterali (4 e 5) disegnati e conteggiati

## Fase 5 — Rifinitura
- [ ] Tema e disegno del mouse curati
- [ ] Scala corretta su monitor ad alta densità (DPI awareness)
- [ ] Esportare i risultati di una sessione (CSV o testo)
- [ ] Icona e informazioni di versione dell'eseguibile
- [ ] Build Release distribuibile (eseguibile unico, CRT statica)
