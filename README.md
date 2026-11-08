# Mouse Tester

Tester per mouse ad alte prestazioni (fino a 8000 Hz e oltre) per Windows, in C++20 con Dear ImGui e DirectX 11.

## Requisiti
- Windows 10 o 11 (64 bit)
- **Visual Studio 2022 o successivo** (la Community è gratuita) con il carico di lavoro **"Sviluppo di applicazioni desktop con C++"**. CMake è già incluso.
- Connessione a internet alla prima compilazione: CMake scarica Dear ImGui da GitHub.

## Compilare
**Da Visual Studio:** File → Apri → Cartella... e scegli questa cartella. Visual Studio riconosce il `CMakeLists.txt`. Scegli la configurazione `x64-Release`, poi seleziona `MouseTester.exe` come destinazione di avvio e premi F5.

**Da riga di comando** ("Developer PowerShell" di Visual Studio):
```
cmake -B build -A x64
cmake --build build --config Release
build\Release\MouseTester.exe
```

## Struttura
| File | Contenuto |
|---|---|
| `src/raw_input.*` | Thread di cattura: Raw Input, timestamp con QueryPerformanceCounter, nome del dispositivo HID |
| `src/spsc_queue.hpp` | Coda lock-free tra il thread di cattura e quello dell'interfaccia |
| `src/analyzer.*` | Statistiche: polling, jitter, continuità del sensore, DPI, chatter, rotella |
| `src/mouse_view.*` | Disegno vettoriale del mouse con i tasti che si illuminano |
| `src/ui.*` | Interfaccia: tema e schede |
| `src/main.cpp` | Finestra Win32, DirectX 11, ciclo principale |

## Note sulle misure
- Tieni la finestra **in primo piano** durante i test: alcune versioni di Windows 11 limitano l'input dei programmi in background.
- Collega il mouse o il ricevitore **direttamente al PC**, non a un hub o al monitor.
- Il timestamp viene preso quando Windows consegna il pacchetto, non quando il mouse lo invia. Il jitter misurato comprende quindi anche USB, driver e scheduler.
- La latenza del click e del sensore e la lift-off distance non si possono misurare via software.
