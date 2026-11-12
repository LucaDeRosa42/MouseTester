// Mouse Tester - Copyright (C) 2026 lcrd42
// SPDX-License-Identifier: GPL-3.0-or-later
#include "i18n.hpp"

#include <windows.h>

#include <cstring>

#include "imgui.h"

namespace {

Lang g_lang = Lang::En;

constexpr int kLangs = int(Lang::Count);
using Row = const char* [kLangs]; // En, It, Ru, Es, Fr, De, Pt, Zh

// Every row must keep the printf placeholders of the English text, in the same order.
const Row kStrings[] = {
    // ---------------------------------------------------------------- left panel
    /* AllDevices */ {"All devices", "Tutti i dispositivi", "Все устройства", "Todos los dispositivos",
                      "Tous les périphériques", "Alle Geräte", "Todos os dispositivos", "所有设备"},
    /* DevicePackets */ {"%s  -  %llu packets", "%s  -  %llu pacchetti", "%s  -  %llu пакетов", "%s  -  %llu paquetes",
                         "%s  -  %llu paquets", "%s  -  %llu Pakete", "%s  -  %llu pacotes", "%s  -  %llu 个数据包"},
    /* Language */ {"Language", "Lingua", "Язык", "Idioma", "Langue", "Sprache", "Idioma", "语言"},
    /* ColButton */ {"Button", "Tasto", "Кнопка", "Botón", "Bouton", "Taste", "Botão", "按键"},
    /* ColPresses */ {"Presses", "Pressioni", "Нажатия", "Pulsaciones", "Appuis", "Klicks", "Cliques", "按下次数"},
    /* ColChatter */ {"Chatter", "Chatter", "Дребезг", "Rebote", "Rebond", "Prellen", "Repique", "连击"},
    /* BtnLeft */ {"Left", "Sinistro", "Левая", "Izquierdo", "Gauche", "Links", "Esquerdo", "左键"},
    /* BtnRight */ {"Right", "Destro", "Правая", "Derecho", "Droit", "Rechts", "Direito", "右键"},
    /* BtnMiddle */ {"Middle", "Centrale", "Средняя", "Central", "Milieu", "Mitte", "Meio", "中键"},
    /* BtnSide4 */ {"Side 4", "Laterale 4", "Боковая 4", "Lateral 4", "Latéral 4", "Seite 4", "Lateral 4", "侧键 4"},
    /* BtnSide5 */ {"Side 5", "Laterale 5", "Боковая 5", "Lateral 5", "Latéral 5", "Seite 5", "Lateral 5", "侧键 5"},
    /* WheelSummary */ {"Wheel:  up %.0f   down %.0f notches", "Rotella:  su %.0f   giù %.0f scatti",
                        "Колесо:  вверх %.0f   вниз %.0f щелчков", "Rueda:  arriba %.0f   abajo %.0f pasos",
                        "Molette :  haut %.0f   bas %.0f crans", "Mausrad:  hoch %.0f   runter %.0f Rasten",
                        "Roda:  cima %.0f   baixo %.0f passos", "滚轮：  上 %.0f   下 %.0f 格"},
    /* ExportReport */ {"Export report...", "Esporta report...", "Экспорт отчёта...", "Exportar informe...",
                        "Exporter le rapport...", "Bericht exportieren...", "Exportar relatório...", "导出报告..."},
    /* ResetAll */ {"Reset all", "Azzera tutto", "Сбросить всё", "Reiniciar todo", "Tout réinitialiser",
                    "Alles zurücksetzen", "Reiniciar tudo", "全部重置"},
    /* ReportSaved */ {"Report saved", "Report salvato", "Отчёт сохранён", "Informe guardado", "Rapport enregistré",
                       "Bericht gespeichert", "Relatório salvo", "报告已保存"},
    /* ReportFailed */ {"Could not save the report", "Impossibile salvare il report", "Не удалось сохранить отчёт",
                        "No se pudo guardar el informe", "Impossible d'enregistrer le rapport",
                        "Bericht konnte nicht gespeichert werden", "Não foi possível salvar o relatório", "无法保存报告"},
    /* IntervalsSaved */ {"Intervals saved", "Intervalli salvati", "Интервалы сохранены", "Intervalos guardados",
                          "Intervalles enregistrés", "Intervalle gespeichert", "Intervalos salvos", "间隔已保存"},
    /* IntervalsFailed */ {"Could not save the intervals", "Impossibile salvare gli intervalli",
                           "Не удалось сохранить интервалы", "No se pudieron guardar los intervalos",
                           "Impossible d'enregistrer les intervalles", "Intervalle konnten nicht gespeichert werden",
                           "Não foi possível salvar os intervalos", "无法保存间隔"},

    // ---------------------------------------------------------------- top bar and tabs
    /* CaptureStats */ {"capture %.1f %% CPU · %.1f µs/packet    frame %.1f ms · worst %.1f ms",
                        "cattura %.1f %% CPU · %.1f µs/pacchetto    frame %.1f ms · peggiore %.1f ms",
                        "захват %.1f %% CPU · %.1f мкс/пакет    кадр %.1f мс · худший %.1f мс",
                        "captura %.1f %% CPU · %.1f µs/paquete    fotograma %.1f ms · peor %.1f ms",
                        "capture %.1f %% CPU · %.1f µs/paquet    image %.1f ms · pire %.1f ms",
                        "Erfassung %.1f %% CPU · %.1f µs/Paket    Frame %.1f ms · max. %.1f ms",
                        "captura %.1f %% CPU · %.1f µs/pacote    quadro %.1f ms · pior %.1f ms",
                        "采集 %.1f %% CPU · %.1f µs/包    帧 %.1f ms · 最差 %.1f ms"},
    /* CaptureIdle */ {"capture idle    frame %.1f ms · worst %.1f ms", "cattura inattiva    frame %.1f ms · peggiore %.1f ms",
                       "захват простаивает    кадр %.1f мс · худший %.1f мс",
                       "captura inactiva    fotograma %.1f ms · peor %.1f ms",
                       "capture inactive    image %.1f ms · pire %.1f ms",
                       "Erfassung inaktiv    Frame %.1f ms · max. %.1f ms",
                       "captura ociosa    quadro %.1f ms · pior %.1f ms", "采集空闲    帧 %.1f ms · 最差 %.1f ms"},
    /* TipPerf */ {
        "Capture: CPU used by the high-priority capture thread, as a percentage of one core, and per packet. "
        "Measured from the thread's own CPU cycles.\n\nFrame: average and worst frame time of the interface over "
        "the last frames. It turns orange when a frame takes much longer than usual (a stutter).",
        "Cattura: CPU usata dal thread di cattura ad alta priorità, in percentuale di un core e per pacchetto. "
        "Misurata dai cicli di CPU del thread stesso.\n\nFrame: tempo medio e peggiore dei frame dell'interfaccia "
        "negli ultimi istanti. Diventa arancione quando un frame dura molto più del solito (uno scatto).",
        "Захват: загрузка CPU потоком захвата с высоким приоритетом, в процентах одного ядра и на пакет. "
        "Измеряется по собственным тактам потока.\n\nКадр: среднее и худшее время кадра интерфейса за последние "
        "кадры. Становится оранжевым, когда кадр длится заметно дольше обычного (подтормаживание).",
        "Captura: CPU usada por el hilo de captura de alta prioridad, como porcentaje de un núcleo y por paquete. "
        "Se mide con los ciclos de CPU del propio hilo.\n\nFotograma: tiempo medio y peor de los últimos "
        "fotogramas de la interfaz. Se vuelve naranja cuando un fotograma tarda mucho más de lo normal (un tirón).",
        "Capture : CPU utilisé par le thread de capture haute priorité, en pourcentage d'un cœur et par paquet. "
        "Mesuré à partir des cycles CPU du thread.\n\nImage : temps moyen et pire des dernières images de "
        "l'interface. Passe en orange quand une image dure bien plus que d'habitude (une saccade).",
        "Erfassung: CPU-Last des hochpriorisierten Erfassungs-Threads, in Prozent eines Kerns und pro Paket. "
        "Gemessen über die eigenen CPU-Zyklen des Threads.\n\nFrame: durchschnittliche und längste Frame-Zeit der "
        "Oberfläche über die letzten Frames. Wird orange, wenn ein Frame deutlich länger dauert (ein Ruckler).",
        "Captura: CPU usada pela thread de captura de alta prioridade, em porcentagem de um núcleo e por pacote. "
        "Medida pelos ciclos de CPU da própria thread.\n\nQuadro: tempo médio e pior dos últimos quadros da "
        "interface. Fica laranja quando um quadro demora muito mais que o normal (um travamento).",
        "采集：高优先级采集线程的 CPU 占用，以单个核心的百分比和每个数据包的时间表示，由线程自身的 CPU 周期测得。\n\n"
        "帧：界面最近若干帧的平均和最长帧时间。当某一帧明显比平时更慢（卡顿）时会变成橙色。"},
    /* Dropped */ {"Warning: %llu packets lost by the internal queue.",
                   "Attenzione: %llu pacchetti persi dalla coda interna.",
                   "Внимание: %llu пакетов потеряно во внутренней очереди.",
                   "Atención: %llu paquetes perdidos en la cola interna.",
                   "Attention : %llu paquets perdus par la file interne.",
                   "Achtung: %llu Pakete in der internen Warteschlange verloren.",
                   "Atenção: %llu pacotes perdidos na fila interna.", "警告：内部队列丢失了 %llu 个数据包。"},
    /* TabPolling */ {"Polling rate", "Polling rate", "Частота опроса", "Tasa de sondeo", "Taux d'interrogation",
                      "Abfragerate", "Taxa de polling", "回报率"},
    /* TabSensor */ {"Sensor", "Sensore", "Сенсор", "Sensor", "Capteur", "Sensor", "Sensor", "传感器"},
    /* TabDpi */ {"DPI", "DPI", "DPI", "DPI", "DPI", "DPI", "DPI", "DPI"},
    /* TabClicks */ {"Clicks & wheel", "Click e rotella", "Клики и колесо", "Clics y rueda", "Clics et molette",
                     "Klicks & Mausrad", "Cliques e roda", "按键与滚轮"},

    // ---------------------------------------------------------------- polling
    /* Nominal */ {"nominal %d Hz", "nominale %d Hz", "номинал %d Гц", "nominal %d Hz", "nominal %d Hz",
                   "nominal %d Hz", "nominal %d Hz", "标称 %d Hz"},
    /* MoveCircles */ {"move the mouse quickly in circles", "muovi il mouse velocemente in cerchio",
                       "быстро двигайте мышь по кругу", "mueve el ratón rápido en círculos",
                       "déplacez la souris rapidement en cercles", "Maus schnell im Kreis bewegen",
                       "mova o mouse rapidamente em círculos", "快速画圈移动鼠标"},
    /* Measuring */ {"Measuring · speed %.1f counts per packet", "In misura · velocità %.1f count per pacchetto",
                     "Измерение · скорость %.1f отсчётов на пакет", "Midiendo · velocidad %.1f cuentas por paquete",
                     "Mesure en cours · vitesse %.1f counts par paquet", "Messung · Tempo %.1f Counts pro Paket",
                     "Medindo · velocidade %.1f contagens por pacote", "测量中 · 速度 每包 %.1f 计数"},
    /* TooSlow */ {"Paused: too slow (%.1f counts per packet, at least %.1f needed). Move faster or raise the DPI.",
                   "In pausa: troppo lento (%.1f count per pacchetto, servono almeno %.1f). Muovi più veloce o alza i DPI.",
                   "Пауза: слишком медленно (%.1f отсчётов на пакет, нужно не меньше %.1f). Двигайте быстрее или повысьте DPI.",
                   "En pausa: demasiado lento (%.1f cuentas por paquete, se necesitan al menos %.1f). Muévelo más rápido o sube los DPI.",
                   "En pause : trop lent (%.1f counts par paquet, au moins %.1f requis). Bougez plus vite ou augmentez les DPI.",
                   "Pausiert: zu langsam (%.1f Counts pro Paket, mindestens %.1f nötig). Schneller bewegen oder DPI erhöhen.",
                   "Pausado: lento demais (%.1f contagens por pacote, mínimo %.1f). Mova mais rápido ou aumente o DPI.",
                   "已暂停：太慢（每包 %.1f 计数，至少需要 %.1f）。请移动得更快或提高 DPI。"},
    /* NotMoving */ {"Paused: mouse not moving", "In pausa: mouse fermo", "Пауза: мышь не движется",
                     "En pausa: el ratón no se mueve", "En pause : la souris ne bouge pas", "Pausiert: Maus steht still",
                     "Pausado: mouse parado", "已暂停：鼠标未移动"},
    /* Mean */ {"Mean", "Media", "Среднее", "Media", "Moyenne", "Mittelwert", "Média", "平均值"},
    /* Median */ {"Median", "Mediana", "Медиана", "Mediana", "Médiane", "Median", "Mediana", "中位数"},
    /* Jitter */ {"Jitter (std. dev.)", "Jitter (dev. std.)", "Джиттер (ст. откл.)", "Jitter (desv. est.)",
                  "Gigue (écart type)", "Jitter (Std.-Abw.)", "Jitter (desvio padrão)", "抖动（标准差）"},
    /* PacketsPerSec */ {"Packets / s", "Pacchetti / s", "Пакетов / с", "Paquetes / s", "Paquets / s", "Pakete / s",
                         "Pacotes / s", "数据包 / 秒"},
    /* MinMax */ {"Min / Max", "Min / Max", "Мин / Макс", "Mín / Máx", "Min / Max", "Min / Max", "Mín / Máx", "最小 / 最大"},
    /* Percentiles */ {"1st / 99th percentile", "1° / 99° percentile", "1-й / 99-й процентиль", "Percentil 1 / 99",
                       "1er / 99e centile", "1. / 99. Perzentil", "Percentil 1 / 99", "第 1 / 99 百分位"},
    /* Outliers */ {"Outliers (>1.5x)", "Fuori norma (>1.5x)", "Выбросы (>1.5x)", "Atípicos (>1.5x)",
                    "Aberrants (>1.5x)", "Ausreißer (>1.5x)", "Atípicos (>1.5x)", "异常值 (>1.5x)"},
    /* Batched */ {"Batched (<0.25x)", "Raggruppati (<0.25x)", "Пачками (<0.25x)", "Agrupados (<0.25x)",
                   "Groupés (<0.25x)", "Gebündelt (<0.25x)", "Agrupados (<0.25x)", "成批 (<0.25x)"},
    /* IntervalDistribution */ {"Interval distribution", "Distribuzione degli intervalli", "Распределение интервалов",
                                "Distribución de intervalos", "Distribution des intervalles", "Verteilung der Intervalle",
                                "Distribuição dos intervalos", "间隔分布"},
    /* LastIntervals */ {"Last 2000 intervals", "Ultimi 2000 intervalli", "Последние 2000 интервалов",
                         "Últimos 2000 intervalos", "2000 derniers intervalles", "Letzte 2000 Intervalle",
                         "Últimos 2000 intervalos", "最近 2000 个间隔"},
    /* HintPollSpeed */ {
        "Only movements fast enough to fill every poll are measured: at least 1.5 counts per packet. At 8000 Hz "
        "and 800 DPI that means about 38 cm/s; at 3200 DPI about 10 cm/s is enough.",
        "La misura usa solo i movimenti abbastanza veloci da riempire ogni poll: almeno 1,5 count per pacchetto. "
        "A 8000 Hz e 800 DPI vuol dire circa 38 cm/s; a 3200 DPI ne bastano circa 10.",
        "Учитываются только движения, достаточно быстрые, чтобы заполнить каждый опрос: не меньше 1,5 отсчёта на "
        "пакет. При 8000 Гц и 800 DPI это около 38 см/с; при 3200 DPI достаточно около 10 см/с.",
        "Solo se miden los movimientos lo bastante rápidos para llenar cada sondeo: al menos 1,5 cuentas por "
        "paquete. A 8000 Hz y 800 DPI son unos 38 cm/s; a 3200 DPI bastan unos 10 cm/s.",
        "Seuls les mouvements assez rapides pour remplir chaque interrogation sont mesurés : au moins 1,5 count "
        "par paquet. À 8000 Hz et 800 DPI cela fait environ 38 cm/s ; à 3200 DPI environ 10 cm/s suffisent.",
        "Gemessen werden nur Bewegungen, die schnell genug sind, um jede Abfrage zu füllen: mindestens 1,5 Counts "
        "pro Paket. Bei 8000 Hz und 800 DPI sind das etwa 38 cm/s; bei 3200 DPI reichen etwa 10 cm/s.",
        "Só são medidos movimentos rápidos o bastante para preencher cada polling: pelo menos 1,5 contagem por "
        "pacote. A 8000 Hz e 800 DPI isso dá cerca de 38 cm/s; a 3200 DPI bastam cerca de 10 cm/s.",
        "只统计快到每次轮询都有位移的移动：每包至少 1.5 计数。在 8000 Hz、800 DPI 下约需 38 cm/s；3200 DPI 下约 10 cm/s 即可。"},
    /* HintBatched */ {
        "\"Batched\" intervals are packets delivered together by Windows: they point to system or USB driver load "
        "rather than a mouse defect. Plug the cable/receiver directly into the PC, not into a hub.",
        "Gli intervalli \"raggruppati\" sono pacchetti consegnati insieme da Windows: indicano carico di sistema o "
        "del driver USB più che un difetto del mouse. Collega cavo o ricevitore direttamente al PC, non a un hub.",
        "Интервалы «пачками» — это пакеты, которые Windows доставила вместе: они говорят о нагрузке на систему или "
        "драйвер USB, а не о дефекте мыши. Подключайте кабель/приёмник напрямую к ПК, а не к хабу.",
        "Los intervalos \"agrupados\" son paquetes entregados juntos por Windows: indican carga del sistema o del "
        "controlador USB más que un defecto del ratón. Conecta el cable/receptor directamente al PC, no a un hub.",
        "Les intervalles « groupés » sont des paquets livrés ensemble par Windows : ils indiquent une charge du "
        "système ou du pilote USB plutôt qu'un défaut de la souris. Branchez le câble/récepteur directement sur le PC, pas sur un hub.",
        "„Gebündelte“ Intervalle sind Pakete, die Windows zusammen ausliefert: Sie deuten auf System- oder "
        "USB-Treiberlast hin, nicht auf einen Mausdefekt. Kabel/Empfänger direkt am PC anschließen, nicht an einem Hub.",
        "Intervalos \"agrupados\" são pacotes entregues juntos pelo Windows: indicam carga do sistema ou do driver "
        "USB, não um defeito do mouse. Ligue o cabo/receptor direto no PC, não em um hub.",
        "“成批”间隔是 Windows 一起投递的数据包：通常说明系统或 USB 驱动负载较高，而不是鼠标故障。请将线缆或接收器直接插在电脑上，不要接集线器。"},
    /* ResetPolling */ {"Reset polling", "Azzera polling", "Сбросить опрос", "Reiniciar sondeo", "Réinitialiser",
                        "Abfrage zurücksetzen", "Reiniciar polling", "重置回报率"},
    /* ExportIntervals */ {"Export intervals (CSV)...", "Esporta intervalli (CSV)...", "Экспорт интервалов (CSV)...",
                           "Exportar intervalos (CSV)...", "Exporter les intervalles (CSV)...",
                           "Intervalle exportieren (CSV)...", "Exportar intervalos (CSV)...", "导出间隔 (CSV)..."},

    // ---------------------------------------------------------------- sensor
    /* PacketsChecked */ {"Packets checked", "Pacchetti valutati", "Проверено пакетов", "Paquetes evaluados",
                          "Paquets analysés", "Geprüfte Pakete", "Pacotes avaliados", "已检测数据包"},
    /* TipChecked */ {
        "Packets checked\n\nPackets moving fast and steadily enough to be judged: at least 3 counts on average, with "
        "consistent neighbours. Slow or irregular movement is skipped, because count rounding would make the "
        "comparison unreliable. The other counters only refer to these packets.",
        "Pacchetti valutati\n\nPacchetti abbastanza veloci e regolari da poter essere giudicati: almeno 3 count in "
        "media, con vicini coerenti. Il movimento lento o irregolare viene saltato, perché l'arrotondamento dei "
        "count renderebbe il confronto inaffidabile. Gli altri contatori si riferiscono solo a questi pacchetti.",
        "Проверено пакетов\n\nПакеты с достаточно быстрым и ровным движением для оценки: в среднем не меньше 3 "
        "отсчётов, с согласованными соседями. Медленное или неровное движение пропускается — округление отсчётов "
        "сделало бы сравнение ненадёжным. Остальные счётчики относятся только к этим пакетам.",
        "Paquetes evaluados\n\nPaquetes con movimiento lo bastante rápido y regular para juzgarlos: al menos 3 "
        "cuentas de media, con vecinos coherentes. El movimiento lento o irregular se omite, porque el redondeo "
        "haría poco fiable la comparación. Los demás contadores se refieren solo a estos paquetes.",
        "Paquets analysés\n\nPaquets dont le mouvement est assez rapide et régulier pour être jugé : au moins 3 "
        "counts en moyenne, avec des voisins cohérents. Le mouvement lent ou irrégulier est ignoré, car l'arrondi "
        "rendrait la comparaison peu fiable. Les autres compteurs ne concernent que ces paquets.",
        "Geprüfte Pakete\n\nPakete mit ausreichend schneller und gleichmäßiger Bewegung: im Mittel mindestens 3 "
        "Counts, mit stimmigen Nachbarn. Langsame oder unregelmäßige Bewegung wird übersprungen, weil die Rundung "
        "den Vergleich unzuverlässig machen würde. Die anderen Zähler beziehen sich nur auf diese Pakete.",
        "Pacotes avaliados\n\nPacotes com movimento rápido e regular o bastante para serem avaliados: pelo menos 3 "
        "contagens em média, com vizinhos coerentes. Movimento lento ou irregular é ignorado, pois o arredondamento "
        "tornaria a comparação pouco confiável. Os outros contadores se referem só a esses pacotes.",
        "已检测数据包\n\n移动足够快且平稳、可以被评判的数据包：平均至少 3 计数，且与相邻包一致。慢速或不规则的移动会被跳过，"
        "因为计数取整会让比较不可靠。其他计数器只统计这些数据包。"},
    /* Missing */ {"Missing", "Mancanti", "Пропущено", "Perdidos", "Manquants", "Fehlend", "Perdidos", "丢失"},
    /* Spikes */ {"Spikes", "Picchi", "Всплески", "Picos", "Pics", "Spitzen", "Picos", "尖峰"},
    /* Dips */ {"Dips", "Cali", "Провалы", "Caídas", "Creux", "Einbrüche", "Quedas", "骤降"},
    /* Reversals */ {"Reversals", "Inversioni", "Развороты", "Inversiones", "Inversions", "Umkehrungen", "Inversões", "反向"},
    /* TipMissing */ {
        "Missing packet\n\nA packet carried 2, 3 or 4 times the movement of its neighbours: the mouse skipped 1-3 "
        "polls and sent the accumulated movement all at once. Typical of a sensor or firmware that cannot keep up "
        "with the polling rate, or of a weak wireless link.",
        "Pacchetto mancante\n\nUn pacchetto ha portato 2, 3 o 4 volte il movimento dei vicini: il mouse ha saltato "
        "1-3 poll e ha inviato tutto il movimento accumulato insieme. Tipico di un sensore o firmware che non regge "
        "il polling rate, o di un collegamento wireless debole.",
        "Пропущенный пакет\n\nПакет содержал в 2, 3 или 4 раза больше движения, чем соседние: мышь пропустила 1-3 "
        "опроса и отправила накопленное движение разом. Типично для сенсора или прошивки, не успевающих за "
        "частотой опроса, или для слабого беспроводного соединения.",
        "Paquete perdido\n\nUn paquete llevó 2, 3 o 4 veces el movimiento de sus vecinos: el ratón se saltó 1-3 "
        "sondeos y envió todo el movimiento acumulado de golpe. Típico de un sensor o firmware que no aguanta la "
        "tasa de sondeo, o de una conexión inalámbrica débil.",
        "Paquet manquant\n\nUn paquet a transporté 2, 3 ou 4 fois le mouvement de ses voisins : la souris a sauté "
        "1 à 3 interrogations et a envoyé tout le mouvement accumulé d'un coup. Typique d'un capteur ou d'un "
        "firmware qui ne suit pas le taux d'interrogation, ou d'une liaison sans fil faible.",
        "Fehlendes Paket\n\nEin Paket enthielt das 2-, 3- oder 4-fache der Bewegung seiner Nachbarn: Die Maus hat "
        "1-3 Abfragen ausgelassen und die angesammelte Bewegung auf einmal gesendet. Typisch für Sensor oder "
        "Firmware, die mit der Abfragerate nicht mithalten, oder für eine schwache Funkverbindung.",
        "Pacote perdido\n\nUm pacote trouxe 2, 3 ou 4 vezes o movimento dos vizinhos: o mouse pulou 1-3 pollings "
        "e enviou todo o movimento acumulado de uma vez. Típico de sensor ou firmware que não acompanha a taxa de "
        "polling, ou de uma conexão sem fio fraca.",
        "丢失数据包\n\n某个数据包携带的位移是相邻包的 2、3 或 4 倍：鼠标跳过了 1-3 次轮询，把累积的位移一次性发出。"
        "常见于跟不上回报率的传感器或固件，或无线信号较弱。"},
    /* TipSpike */ {
        "Spike\n\nA packet carried much more movement than its neighbours (at least 1.6x), but not an exact "
        "multiple, so it is not explained by missing packets. It is a sudden jump of the sensor reading: dust on "
        "the lens, a bad surface or a sensor glitch.",
        "Picco\n\nUn pacchetto ha portato molto più movimento dei vicini (almeno 1,6 volte), ma non un multiplo "
        "esatto, quindi non si spiega con pacchetti mancanti. È un salto improvviso della lettura del sensore: "
        "polvere sulla lente, una superficie difficile o un difetto del sensore.",
        "Всплеск\n\nПакет содержал намного больше движения, чем соседние (не меньше чем в 1,6 раза), но не точное "
        "кратное, поэтому это не пропуск пакетов. Это резкий скачок показаний сенсора: пыль на линзе, плохая "
        "поверхность или сбой сенсора.",
        "Pico\n\nUn paquete llevó mucho más movimiento que sus vecinos (al menos 1,6 veces), pero no un múltiplo "
        "exacto, así que no se explica por paquetes perdidos. Es un salto brusco de la lectura del sensor: polvo "
        "en la lente, una mala superficie o un fallo del sensor.",
        "Pic\n\nUn paquet a transporté bien plus de mouvement que ses voisins (au moins 1,6 fois), mais pas un "
        "multiple exact : ce ne sont donc pas des paquets manquants. C'est un saut brusque de la lecture du "
        "capteur : poussière sur la lentille, mauvaise surface ou défaillance du capteur.",
        "Spitze\n\nEin Paket enthielt deutlich mehr Bewegung als seine Nachbarn (mindestens 1,6-fach), aber kein "
        "exaktes Vielfaches, also keine fehlenden Pakete. Es ist ein plötzlicher Sprung im Sensorwert: Staub auf "
        "der Linse, eine schlechte Oberfläche oder ein Sensorfehler.",
        "Pico\n\nUm pacote trouxe muito mais movimento que os vizinhos (pelo menos 1,6 vez), mas não um múltiplo "
        "exato, então não se explica por pacotes perdidos. É um salto repentino na leitura do sensor: poeira na "
        "lente, superfície ruim ou falha do sensor.",
        "尖峰\n\n某个数据包的位移远大于相邻包（至少 1.6 倍），但又不是整数倍，因此不是丢包造成的。这是传感器读数的突然跳变："
        "镜头有灰尘、表面不合适或传感器故障。"},
    /* TipDip */ {
        "Dip\n\nA packet carried much less movement than its neighbours (below 0.45x) while the mouse was moving "
        "steadily. The sensor briefly lost tracking or reported late, often because of the surface or lift-off.",
        "Calo\n\nUn pacchetto ha portato molto meno movimento dei vicini (meno di 0,45 volte) mentre il mouse si "
        "muoveva in modo regolare. Il sensore ha perso per un istante il tracciamento o ha riportato in ritardo, "
        "spesso per colpa della superficie o del sollevamento.",
        "Провал\n\nПакет содержал намного меньше движения, чем соседние (меньше 0,45), при ровном движении мыши. "
        "Сенсор на мгновение потерял отслеживание или передал данные с опозданием — часто из-за поверхности или отрыва.",
        "Caída\n\nUn paquete llevó mucho menos movimiento que sus vecinos (menos de 0,45 veces) mientras el ratón "
        "se movía de forma regular. El sensor perdió el seguimiento un instante o informó tarde, a menudo por la "
        "superficie o al levantar el ratón.",
        "Creux\n\nUn paquet a transporté bien moins de mouvement que ses voisins (moins de 0,45 fois) alors que la "
        "souris bougeait régulièrement. Le capteur a brièvement perdu le suivi ou a répondu en retard, souvent à "
        "cause de la surface ou d'un soulèvement.",
        "Einbruch\n\nEin Paket enthielt deutlich weniger Bewegung als seine Nachbarn (unter 0,45-fach), obwohl die "
        "Maus gleichmäßig bewegt wurde. Der Sensor hat kurz die Verfolgung verloren oder verspätet gemeldet, oft "
        "wegen der Oberfläche oder beim Anheben.",
        "Queda\n\nUm pacote trouxe muito menos movimento que os vizinhos (menos de 0,45 vez) enquanto o mouse se "
        "movia de forma regular. O sensor perdeu o rastreamento por um instante ou informou atrasado, muitas vezes "
        "por causa da superfície ou ao levantar o mouse.",
        "骤降\n\n在鼠标平稳移动时，某个数据包的位移远小于相邻包（低于 0.45 倍）。传感器短暂丢失追踪或延迟上报，通常与表面或抬起有关。"},
    /* TipReversal */ {
        "Reversal\n\nTwo consecutive fast packets point in opposite directions. At 1000 Hz and above a hand cannot "
        "stop and move back within one poll, so this is a sensor error (jitter or a bad reading), not real movement.",
        "Inversione\n\nDue pacchetti veloci consecutivi puntano in direzioni opposte. A 1000 Hz e oltre una mano non "
        "può fermarsi e tornare indietro tra un poll e l'altro, quindi è un errore del sensore (jitter o lettura "
        "sbagliata), non un movimento reale.",
        "Разворот\n\nДва быстрых пакета подряд направлены в противоположные стороны. При 1000 Гц и выше рука не "
        "может остановиться и двинуться назад за один опрос, поэтому это ошибка сенсора (джиттер или неверное "
        "чтение), а не реальное движение.",
        "Inversión\n\nDos paquetes rápidos consecutivos apuntan en direcciones opuestas. A 1000 Hz o más una mano "
        "no puede detenerse y volver atrás entre un sondeo y el siguiente, así que es un error del sensor (jitter "
        "o una mala lectura), no un movimiento real.",
        "Inversion\n\nDeux paquets rapides consécutifs vont dans des directions opposées. À 1000 Hz et plus, une "
        "main ne peut pas s'arrêter et repartir en arrière entre deux interrogations : c'est une erreur du capteur "
        "(gigue ou mauvaise lecture), pas un vrai mouvement.",
        "Umkehrung\n\nZwei aufeinanderfolgende schnelle Pakete zeigen in entgegengesetzte Richtungen. Ab 1000 Hz "
        "kann eine Hand nicht innerhalb einer Abfrage anhalten und zurückfahren, daher ist das ein Sensorfehler "
        "(Jitter oder Fehlmessung), keine echte Bewegung.",
        "Inversão\n\nDois pacotes rápidos consecutivos apontam em direções opostas. A partir de 1000 Hz a mão não "
        "consegue parar e voltar entre um polling e outro, então é um erro do sensor (jitter ou leitura errada), "
        "não um movimento real.",
        "反向\n\n两个连续的快速数据包方向相反。在 1000 Hz 及以上，手不可能在一次轮询内停下并反向移动，因此这是传感器错误"
        "（抖动或误读），而不是真实的移动。"},
    /* MarkMissing */ {"missing", "mancante", "пропуск", "perdido", "manquant", "fehlend", "perdido", "丢失"},
    /* MarkSpike */ {"spike", "picco", "всплеск", "pico", "pic", "Spitze", "pico", "尖峰"},
    /* MarkDip */ {"dip", "calo", "провал", "caída", "creux", "Einbruch", "queda", "骤降"},
    /* MarkReversal */ {"reversal", "inversione", "разворот", "inversión", "inversion", "Umkehrung", "inversão", "反向"},
    /* LastStroke */ {"Last stroke:", "Ultimo tratto:", "Последний штрих:", "Último trazo:", "Dernier tracé :",
                      "Letzter Strich:", "Último traço:", "最近一笔："},
    /* StrokeInfo */ {
        "%.0f counts at %.1f°  ·  deviation %.1f counts RMS (%.2f %%)  ·  nearest guide %.0f° (%+.1f°)",
        "%.0f count a %.1f°  ·  scostamento %.1f count RMS (%.2f %%)  ·  guida più vicina %.0f° (%+.1f°)",
        "%.0f отсчётов под %.1f°  ·  отклонение %.1f отсч. RMS (%.2f %%)  ·  ближайшая направляющая %.0f° (%+.1f°)",
        "%.0f cuentas a %.1f°  ·  desviación %.1f cuentas RMS (%.2f %%)  ·  guía más cercana %.0f° (%+.1f°)",
        "%.0f counts à %.1f°  ·  écart %.1f counts RMS (%.2f %%)  ·  guide la plus proche %.0f° (%+.1f°)",
        "%.0f Counts bei %.1f°  ·  Abweichung %.1f Counts RMS (%.2f %%)  ·  nächste Hilfslinie %.0f° (%+.1f°)",
        "%.0f contagens a %.1f°  ·  desvio %.1f contagens RMS (%.2f %%)  ·  guia mais próxima %.0f° (%+.1f°)",
        "%.0f 计数，角度 %.1f°  ·  偏差 %.1f 计数 RMS (%.2f %%)  ·  最近参考线 %.0f° (%+.1f°)"},
    /* PossibleSnapping */ {"  possible angle snapping", "  possibile angle snapping", "  возможна коррекция прямых",
                            "  posible angle snapping", "  angle snapping probable", "  mögliches Angle Snapping",
                            "  possível angle snapping", "  可能开启了直线修正"},
    /* TipStroke */ {
        "Last stroke\n\nThe path between two pauses is fitted with a straight line. Deviation is how far, on average, "
        "the points are from that line.\n\nTo check for angle snapping, draw slow freehand lines close to 0°, 45° "
        "and 90°. A hand always wobbles by a few counts over a long stroke; if the deviation is close to zero and "
        "the angle sits exactly on a guide, the mouse is straightening the line (angle snapping is on).",
        "Ultimo tratto\n\nIl percorso tra due pause viene approssimato con una retta. Lo scostamento indica quanto, "
        "in media, i punti si allontanano da quella retta.\n\nPer verificare l'angle snapping traccia a mano libera "
        "linee lente vicino a 0°, 45° e 90°. Una mano oscilla sempre di qualche count su un tratto lungo; se lo "
        "scostamento è quasi zero e l'angolo cade esattamente su una guida, il mouse sta raddrizzando la linea "
        "(angle snapping attivo).",
        "Последний штрих\n\nПуть между двумя паузами аппроксимируется прямой. Отклонение показывает, насколько в "
        "среднем точки удалены от этой прямой.\n\nЧтобы проверить коррекцию прямых (angle snapping), медленно "
        "рисуйте от руки линии около 0°, 45° и 90°. Рука на длинном штрихе всегда колеблется на несколько "
        "отсчётов; если отклонение почти нулевое, а угол точно на направляющей, мышь выпрямляет линию.",
        "Último trazo\n\nEl recorrido entre dos pausas se ajusta con una recta. La desviación indica cuánto se "
        "alejan de media los puntos de esa recta.\n\nPara comprobar el angle snapping, dibuja a mano alzada líneas "
        "lentas cerca de 0°, 45° y 90°. Una mano siempre oscila unas cuentas en un trazo largo; si la desviación "
        "es casi cero y el ángulo cae justo sobre una guía, el ratón está enderezando la línea (angle snapping activo).",
        "Dernier tracé\n\nLe trajet entre deux pauses est approché par une droite. L'écart indique à quelle "
        "distance moyenne les points se trouvent de cette droite.\n\nPour vérifier l'angle snapping, tracez à main "
        "levée des lignes lentes proches de 0°, 45° et 90°. Une main oscille toujours de quelques counts sur un "
        "long tracé ; si l'écart est presque nul et que l'angle tombe pile sur un guide, la souris redresse la "
        "ligne (angle snapping actif).",
        "Letzter Strich\n\nDer Weg zwischen zwei Pausen wird durch eine Gerade angenähert. Die Abweichung gibt an, "
        "wie weit die Punkte im Mittel von dieser Geraden entfernt sind.\n\nUm Angle Snapping zu prüfen, langsam "
        "freihändig Linien nahe 0°, 45° und 90° ziehen. Eine Hand schwankt auf einem langen Strich immer um einige "
        "Counts; ist die Abweichung nahezu null und liegt der Winkel genau auf einer Hilfslinie, begradigt die Maus "
        "die Linie (Angle Snapping aktiv).",
        "Último traço\n\nO percurso entre duas pausas é ajustado a uma reta. O desvio indica, em média, quanto os "
        "pontos se afastam dessa reta.\n\nPara verificar o angle snapping, desenhe à mão livre linhas lentas perto "
        "de 0°, 45° e 90°. A mão sempre oscila algumas contagens num traço longo; se o desvio for quase zero e o "
        "ângulo cair exatamente numa guia, o mouse está endireitando a linha (angle snapping ativo).",
        "最近一笔\n\n两次停顿之间的轨迹会被拟合成一条直线。偏差表示各点平均偏离这条直线的距离。\n\n"
        "检测直线修正（angle snapping）时，请徒手慢慢画出接近 0°、45° 和 90° 的线。手在长线段上总会有几个计数的抖动；"
        "如果偏差几乎为零且角度正好落在参考线上，说明鼠标在自动拉直线条（直线修正已开启）。"},
    /* StrokeNone */ {"Last stroke: draw a line of at least 100 counts to measure its angle and straightness.",
                      "Ultimo tratto: traccia una linea di almeno 100 count per misurarne angolo e rettilineità.",
                      "Последний штрих: нарисуйте линию длиной не меньше 100 отсчётов, чтобы измерить угол и прямолинейность.",
                      "Último trazo: dibuja una línea de al menos 100 cuentas para medir su ángulo y rectitud.",
                      "Dernier tracé : tracez une ligne d'au moins 100 counts pour mesurer son angle et sa rectitude.",
                      "Letzter Strich: Ziehe eine Linie von mindestens 100 Counts, um Winkel und Geradheit zu messen.",
                      "Último traço: desenhe uma linha de pelo menos 100 contagens para medir ângulo e retidão.",
                      "最近一笔：画一条至少 100 计数的线，以测量其角度和直线度。"},
    /* Path */ {"Path", "Tracciato", "Траектория", "Trayectoria", "Tracé", "Pfad", "Trajetória", "轨迹"},
    /* Fit */ {"Fit", "Adatta", "Вписать", "Ajustar", "Ajuster", "Einpassen", "Ajustar", "适应窗口"},
    /* RecentOnly */ {"Recent movements only", "Solo ultimi movimenti", "Только последние движения",
                      "Solo movimientos recientes", "Mouvements récents seulement", "Nur letzte Bewegungen",
                      "Só movimentos recentes", "仅最近移动"},
    /* SnapGuides */ {"Snapping guides", "Guide di snapping", "Направляющие", "Guías de snapping", "Guides de snapping",
                      "Snapping-Hilfslinien", "Guias de snapping", "参考线"},
    /* Zoom */ {"  zoom %.2f px/count%s", "  zoom %.2f px/count%s", "  масштаб %.2f пикс/отсч%s",
                "  zoom %.2f px/cuenta%s", "  zoom %.2f px/count%s", "  Zoom %.2f px/Count%s",
                "  zoom %.2f px/contagem%s", "  缩放 %.2f 像素/计数%s"},
    /* ZoomAuto */ {" (auto)", " (auto)", " (авто)", " (auto)", " (auto)", " (auto)", " (auto)", "（自动）"},
    /* Grid */ {"grid %.0f counts", "griglia %.0f count", "сетка %.0f отсч.", "cuadrícula %.0f cuentas",
                "grille %.0f counts", "Raster %.0f Counts", "grade %.0f contagens", "网格 %.0f 计数"},
    /* HintPath */ {"Wheel over the path: zoom. Drag: pan. Double click: fit. Up close, every dot is one packet.",
                    "Rotella sul tracciato: zoom. Trascina: sposta. Doppio click: adatta. Da vicino ogni punto è un pacchetto.",
                    "Колесо над траекторией: масштаб. Перетаскивание: сдвиг. Двойной щелчок: вписать. Вблизи каждая точка — один пакет.",
                    "Rueda sobre la trayectoria: zoom. Arrastrar: mover. Doble clic: ajustar. De cerca, cada punto es un paquete.",
                    "Molette sur le tracé : zoom. Glisser : déplacer. Double clic : ajuster. De près, chaque point est un paquet.",
                    "Mausrad über dem Pfad: Zoom. Ziehen: verschieben. Doppelklick: einpassen. Aus der Nähe ist jeder Punkt ein Paket.",
                    "Roda sobre a trajetória: zoom. Arrastar: mover. Clique duplo: ajustar. De perto, cada ponto é um pacote.",
                    "在轨迹上滚动滚轮：缩放。拖动：平移。双击：适应窗口。放大后每个点就是一个数据包。"},
    /* HintContinuity */ {
        "Continuity: move the mouse fast and steadily. Every packet is compared with its 4 neighbours: if it carries "
        "2-4 times their movement, packets are missing; if it carries much more or much less, it is a spike or a "
        "dip. With a healthy sensor the counters stay at zero. Angle snapping: draw slow freehand lines near 0°, 45° "
        "and 90° and read \"Last stroke\" above. The path uses raw sensor counts, without Windows acceleration.",
        "Continuità: muovi il mouse veloce e in modo regolare. Ogni pacchetto viene confrontato con i 4 vicini: se "
        "porta 2-4 volte il loro movimento mancano dei pacchetti, se ne porta molto di più o molto di meno è un "
        "picco o un calo. Con un sensore sano i contatori restano a zero. Angle snapping: traccia a mano libera "
        "linee lente vicino a 0°, 45° e 90° e leggi \"Ultimo tratto\" qui sopra. Il tracciato usa i count grezzi "
        "del sensore, senza l'accelerazione di Windows.",
        "Непрерывность: двигайте мышь быстро и ровно. Каждый пакет сравнивается с 4 соседними: если в нём в 2-4 "
        "раза больше движения, пакеты пропущены; если намного больше или меньше — это всплеск или провал. У "
        "исправного сенсора счётчики остаются на нуле. Коррекция прямых: медленно рисуйте от руки линии около 0°, "
        "45° и 90° и смотрите «Последний штрих» выше. Траектория строится по сырым отсчётам сенсора, без ускорения Windows.",
        "Continuidad: mueve el ratón rápido y de forma regular. Cada paquete se compara con sus 4 vecinos: si lleva "
        "2-4 veces su movimiento, faltan paquetes; si lleva mucho más o mucho menos, es un pico o una caída. Con "
        "un sensor sano los contadores se quedan en cero. Angle snapping: dibuja a mano alzada líneas lentas cerca "
        "de 0°, 45° y 90° y mira \"Último trazo\" arriba. La trayectoria usa las cuentas en bruto del sensor, sin la "
        "aceleración de Windows.",
        "Continuité : déplacez la souris vite et régulièrement. Chaque paquet est comparé à ses 4 voisins : s'il "
        "transporte 2 à 4 fois leur mouvement, des paquets manquent ; s'il en transporte beaucoup plus ou beaucoup "
        "moins, c'est un pic ou un creux. Avec un capteur sain, les compteurs restent à zéro. Angle snapping : "
        "tracez à main levée des lignes lentes proches de 0°, 45° et 90° et lisez « Dernier tracé » plus haut. Le "
        "tracé utilise les counts bruts du capteur, sans l'accélération de Windows.",
        "Kontinuität: Maus schnell und gleichmäßig bewegen. Jedes Paket wird mit seinen 4 Nachbarn verglichen: "
        "Enthält es das 2- bis 4-fache ihrer Bewegung, fehlen Pakete; enthält es viel mehr oder viel weniger, ist "
        "es eine Spitze oder ein Einbruch. Bei einem intakten Sensor bleiben die Zähler auf null. Angle Snapping: "
        "langsam freihändig Linien nahe 0°, 45° und 90° ziehen und oben „Letzter Strich“ ablesen. Der Pfad nutzt "
        "die rohen Sensor-Counts, ohne Windows-Beschleunigung.",
        "Continuidade: mova o mouse rápido e de forma regular. Cada pacote é comparado com os 4 vizinhos: se traz "
        "2-4 vezes o movimento deles, faltam pacotes; se traz muito mais ou muito menos, é um pico ou uma queda. "
        "Com um sensor saudável os contadores ficam em zero. Angle snapping: desenhe à mão livre linhas lentas "
        "perto de 0°, 45° e 90° e veja \"Último traço\" acima. A trajetória usa as contagens brutas do sensor, sem a "
        "aceleração do Windows.",
        "连续性：快速且平稳地移动鼠标。每个数据包都会与相邻的 4 个包比较：如果位移是它们的 2-4 倍，说明有丢包；"
        "如果多得多或少得多，就是尖峰或骤降。传感器正常时计数器保持为零。直线修正：徒手慢慢画出接近 0°、45° 和 90° "
        "的线，并查看上方的“最近一笔”。轨迹使用传感器原始计数，不受 Windows 加速影响。"},
    /* ResetSensor */ {"Reset sensor", "Azzera sensore", "Сбросить сенсор", "Reiniciar sensor", "Réinitialiser",
                       "Sensor zurücksetzen", "Reiniciar sensor", "重置传感器"},

    // ---------------------------------------------------------------- DPI
    /* HintDpi */ {
        "1. Lay a ruler (or the edge of the mousepad) and enter its length.  2. Put the mouse at the start mark and "
        "the cursor over the pad below.  3. Hold the left button, slide the mouse in a straight line to the end "
        "mark, release.  Repeat 5-10 times: every release adds a pass.",
        "1. Appoggia un righello (o usa il bordo del tappetino) e inseriscine la lunghezza.  2. Metti il mouse "
        "all'inizio e il cursore sull'area qui sotto.  3. Tieni premuto il tasto sinistro, fai scorrere il mouse "
        "in linea retta fino alla fine e rilascia.  Ripeti 5-10 volte: ogni rilascio aggiunge una passata.",
        "1. Положите линейку (или используйте край коврика) и введите её длину.  2. Поставьте мышь на начальную "
        "отметку, а курсор — на поле ниже.  3. Удерживая левую кнопку, проведите мышь по прямой до конечной "
        "отметки и отпустите.  Повторите 5-10 раз: каждое отпускание добавляет проход.",
        "1. Coloca una regla (o usa el borde de la alfombrilla) e introduce su longitud.  2. Pon el ratón en la "
        "marca inicial y el cursor sobre el área de abajo.  3. Mantén pulsado el botón izquierdo, desliza el ratón "
        "en línea recta hasta la marca final y suelta.  Repite 5-10 veces: cada vez que sueltas se añade una pasada.",
        "1. Posez une règle (ou utilisez le bord du tapis) et saisissez sa longueur.  2. Placez la souris au repère "
        "de départ et le curseur sur la zone ci-dessous.  3. Maintenez le bouton gauche, faites glisser la souris "
        "en ligne droite jusqu'au repère de fin, relâchez.  Répétez 5 à 10 fois : chaque relâchement ajoute un passage.",
        "1. Lineal anlegen (oder die Kante des Mauspads nutzen) und die Länge eingeben.  2. Maus an die Startmarke "
        "und den Cursor über das Feld unten setzen.  3. Linke Taste halten, die Maus gerade bis zur Endmarke "
        "schieben, loslassen.  5-10 Mal wiederholen: jedes Loslassen fügt einen Durchgang hinzu.",
        "1. Coloque uma régua (ou use a borda do mousepad) e digite o comprimento.  2. Ponha o mouse na marca "
        "inicial e o cursor sobre a área abaixo.  3. Segure o botão esquerdo, deslize o mouse em linha reta até a "
        "marca final e solte.  Repita 5-10 vezes: cada vez que soltar, uma passada é adicionada.",
        "1. 放一把尺子（或使用鼠标垫边缘）并输入其长度。  2. 将鼠标放在起点，光标移到下方区域上。  3. 按住左键，"
        "沿直线将鼠标滑到终点后松开。  重复 5-10 次：每次松开都会增加一次测量。"},
    /* Distance */ {"Distance", "Distanza", "Расстояние", "Distancia", "Distance", "Strecke", "Distância", "距离"},
    /* DpiSet */ {"DPI set on the mouse", "DPI impostato nel mouse", "DPI, заданный в мыши", "DPI configurado en el ratón",
                  "DPI réglé sur la souris", "In der Maus eingestellte DPI", "DPI configurado no mouse", "鼠标设定的 DPI"},
    /* Recording */ {"Recording...  %.0f counts  (%.0f DPI so far)", "Registrazione...  %.0f count  (%.0f DPI finora)",
                     "Запись...  %.0f отсчётов  (пока %.0f DPI)", "Grabando...  %.0f cuentas  (%.0f DPI hasta ahora)",
                     "Enregistrement...  %.0f counts  (%.0f DPI pour l'instant)", "Aufnahme...  %.0f Counts  (bisher %.0f DPI)",
                     "Gravando...  %.0f contagens  (%.0f DPI até agora)", "记录中...  %.0f 计数  （当前 %.0f DPI）"},
    /* PadHover */ {"Hold the left button and slide along the ruler", "Tieni premuto il tasto sinistro e scorri lungo il righello",
                    "Удерживайте левую кнопку и ведите вдоль линейки", "Mantén pulsado el botón izquierdo y desliza a lo largo de la regla",
                    "Maintenez le bouton gauche et glissez le long de la règle", "Linke Taste halten und am Lineal entlangschieben",
                    "Segure o botão esquerdo e deslize ao longo da régua", "按住左键，沿尺子滑动"},
    /* PadIdle */ {"Move the cursor here to measure", "Porta qui il cursore per misurare", "Наведите курсор сюда для измерения",
                   "Mueve el cursor aquí para medir", "Placez le curseur ici pour mesurer", "Cursor zum Messen hierher bewegen",
                   "Mova o cursor para cá para medir", "将光标移到这里进行测量"},
    /* AveragePasses */ {"average, passes: %zu", "media, passate: %zu", "среднее, проходов: %zu", "media, pasadas: %zu",
                         "moyenne, passages : %zu", "Mittelwert, Durchgänge: %zu", "média, passadas: %zu", "平均值，测量次数：%zu"},
    /* NoPasses */ {"no passes yet", "nessuna passata", "проходов пока нет", "aún no hay pasadas", "aucun passage",
                    "noch keine Durchgänge", "nenhuma passada ainda", "尚无测量"},
    /* DevFromSet */ {"Deviation from set DPI", "Scostamento dal DPI impostato", "Отклонение от заданного DPI",
                      "Desviación del DPI configurado", "Écart avec le DPI réglé", "Abweichung von eingestellten DPI",
                      "Desvio do DPI configurado", "与设定 DPI 的偏差"},
    /* Spread */ {"Spread (std. dev.)", "Dispersione (dev. std.)", "Разброс (ст. откл.)", "Dispersión (desv. est.)",
                  "Dispersion (écart type)", "Streuung (Std.-Abw.)", "Dispersão (desvio padrão)", "离散度（标准差）"},
    /* Passes */ {"Passes", "Passate", "Проходы", "Pasadas", "Passages", "Durchgänge", "Passadas", "测量次数"},
    /* ColCountsX */ {"Counts X", "Count X", "Отсчёты X", "Cuentas X", "Counts X", "Counts X", "Contagens X", "X 计数"},
    /* ColCountsY */ {"Counts Y", "Count Y", "Отсчёты Y", "Cuentas Y", "Counts Y", "Counts Y", "Contagens Y", "Y 计数"},
    /* ColAngle */ {"Angle", "Angolo", "Угол", "Ángulo", "Angle", "Winkel", "Ângulo", "角度"},
    /* ColDeviation */ {"Deviation", "Scostamento", "Отклонение", "Desviación", "Écart", "Abweichung", "Desvio", "偏差"},
    /* RemoveLast */ {"Remove last pass", "Elimina ultima passata", "Удалить последний проход", "Quitar última pasada",
                      "Supprimer le dernier passage", "Letzten Durchgang entfernen", "Remover última passada", "删除最后一次"},
    /* ClearPasses */ {"Clear passes", "Cancella passate", "Очистить проходы", "Borrar pasadas", "Effacer les passages",
                       "Durchgänge löschen", "Limpar passadas", "清除全部"},

    // ---------------------------------------------------------------- clicks & wheel
    /* ChatterThreshold */ {"Chatter threshold", "Soglia chatter", "Порог дребезга", "Umbral de rebote", "Seuil de rebond",
                            "Prell-Schwelle", "Limite de repique", "连击阈值"},
    /* HintChatter */ {
        "A new press arriving within the threshold after the previous release is counted as an unintended double "
        "click (worn switch or debounce set too low).",
        "Una nuova pressione che arriva entro la soglia dal rilascio precedente viene contata come doppio click "
        "involontario (switch usurato o debounce troppo basso).",
        "Новое нажатие, пришедшее в пределах порога после предыдущего отпускания, считается случайным двойным "
        "щелчком (изношенный переключатель или слишком малый debounce).",
        "Una nueva pulsación que llega dentro del umbral tras la liberación anterior se cuenta como doble clic "
        "involuntario (switch desgastado o debounce demasiado bajo).",
        "Un nouvel appui arrivant dans le seuil après le relâchement précédent est compté comme double clic "
        "involontaire (switch usé ou anti-rebond trop bas).",
        "Ein neuer Klick innerhalb der Schwelle nach dem letzten Loslassen zählt als ungewollter Doppelklick "
        "(abgenutzter Schalter oder zu niedriges Debounce).",
        "Um novo clique que chega dentro do limite após a soltura anterior é contado como clique duplo involuntário "
        "(switch gasto ou debounce baixo demais).",
        "在上次松开后阈值时间内出现的新按下会被计为意外双击（微动老化或去抖时间设置过低）。"},
    /* ColState */ {"State", "Stato", "Состояние", "Estado", "État", "Zustand", "Estado", "状态"},
    /* ColReleasePress */ {"Release → press", "Rilascio → pressione", "Отпускание → нажатие", "Soltar → pulsar",
                           "Relâché → appui", "Loslassen → Klick", "Soltar → clicar", "松开 → 按下"},
    /* ColHold */ {"Hold time", "Durata pressione", "Длительность нажатия", "Tiempo pulsado", "Durée d'appui",
                   "Haltezeit", "Tempo pressionado", "按住时长"},
    /* Pressed */ {"PRESSED", "PREMUTO", "НАЖАТА", "PULSADO", "APPUYÉ", "GEDRÜCKT", "PRESSIONADO", "按下"},
    /* Released */ {"released", "rilasciato", "отпущена", "suelto", "relâché", "losgelassen", "solto", "松开"},
    /* ChatterLast */ {"%llu (last %.1f ms)", "%llu (ultimo %.1f ms)", "%llu (последний %.1f мс)", "%llu (último %.1f ms)",
                       "%llu (dernier %.1f ms)", "%llu (zuletzt %.1f ms)", "%llu (último %.1f ms)", "%llu（最近 %.1f ms）"},
    /* Wheel */ {"Wheel", "Rotella", "Колесо", "Rueda", "Molette", "Mausrad", "Roda", "滚轮"},
    /* NotchesUp */ {"Notches up", "Scatti su", "Щелчков вверх", "Pasos arriba", "Crans haut", "Rasten hoch",
                     "Passos para cima", "向上格数"},
    /* NotchesDown */ {"Notches down", "Scatti giù", "Щелчков вниз", "Pasos abajo", "Crans bas", "Rasten runter",
                       "Passos para baixo", "向下格数"},
    /* QuickReversals */ {"Quick reversals (<40 ms)", "Inversioni rapide (<40 ms)", "Быстрые развороты (<40 мс)",
                          "Inversiones rápidas (<40 ms)", "Inversions rapides (<40 ms)", "Schnelle Umkehrungen (<40 ms)",
                          "Inversões rápidas (<40 ms)", "快速反向 (<40 ms)"},
    /* Tilt */ {"Tilt left / right", "Tilt sx / dx", "Наклон влево / вправо", "Inclinación izq. / der.",
                "Inclinaison gauche / droite", "Neigung links / rechts", "Inclinação esq. / dir.", "左 / 右倾斜"},
    /* HintWheel */ {
        "Scroll the wheel slowly in one direction: every notch should count as 1 and reversals should stay at zero. "
        "Decimal values mean a high-resolution wheel.",
        "Gira la rotella lentamente in una sola direzione: ogni scatto deve contare 1 e le inversioni devono restare "
        "a zero. Valori decimali indicano una rotella ad alta risoluzione.",
        "Медленно крутите колесо в одну сторону: каждый щелчок должен считаться как 1, а развороты оставаться на "
        "нуле. Дробные значения означают колесо высокого разрешения.",
        "Gira la rueda despacio en una sola dirección: cada paso debe contar 1 y las inversiones deben quedarse en "
        "cero. Los valores decimales indican una rueda de alta resolución.",
        "Tournez la molette lentement dans un seul sens : chaque cran doit compter 1 et les inversions doivent "
        "rester à zéro. Des valeurs décimales indiquent une molette haute résolution.",
        "Mausrad langsam in eine Richtung drehen: Jede Raste sollte als 1 zählen und Umkehrungen sollten bei null "
        "bleiben. Dezimalwerte bedeuten ein hochauflösendes Mausrad.",
        "Gire a roda devagar em uma só direção: cada passo deve contar 1 e as inversões devem ficar em zero. "
        "Valores decimais indicam uma roda de alta resolução.",
        "朝一个方向慢慢滚动滚轮：每一格应计为 1，反向次数应保持为零。出现小数说明是高精度滚轮。"},
    /* ClickMove */ {"Movement on click", "Movimento al click", "Смещение при клике", "Movimiento al hacer clic",
                     "Mouvement au clic", "Bewegung beim Klicken", "Movimento ao clicar", "点击时的位移"},
    /* ClickMoveMeasured */ {"Clicks measured", "Click misurati", "Измерено кликов", "Clics medidos",
                             "Clics mesurés", "Gemessene Klicks", "Cliques medidos", "已测点击"},
    /* ClickMoveMoved */ {"With movement", "Con movimento", "Со смещением", "Con movimiento",
                          "Avec mouvement", "Mit Bewegung", "Com movimento", "有位移"},
    /* ClickMoveAvgMax */ {"Average / max (counts)", "Media / max (count)", "Среднее / макс. (counts)",
                           "Media / máx. (counts)", "Moyenne / max (counts)", "Mittel / max (Counts)",
                           "Média / máx. (counts)", "平均 / 最大 (counts)"},
    /* ClickMoveLast */ {"Last click (counts)", "Ultimo click (count)", "Последний клик (counts)",
                         "Último clic (counts)", "Dernier clic (counts)", "Letzter Klick (Counts)",
                         "Último clique (counts)", "上次点击 (counts)"},
    /* HintClickMove */ {
        "Keep the mouse still and click normally: a good mouse reports 0 counts. Only clicks made after 100 ms "
        "without movement and shorter than 300 ms are measured, so drags are ignored.",
        "Tieni il mouse fermo e clicca normalmente: un buon mouse riporta 0 count. Vengono misurati solo i click "
        "fatti dopo 100 ms senza movimento e più brevi di 300 ms, quindi i trascinamenti sono ignorati.",
        "Держите мышь неподвижно и кликайте как обычно: хорошая мышь показывает 0 counts. Измеряются только клики "
        "после 100 мс без движения и короче 300 мс, поэтому перетаскивания не учитываются.",
        "Mantén el ratón quieto y haz clic con normalidad: un buen ratón marca 0 counts. Solo se miden los clics "
        "hechos tras 100 ms sin movimiento y de menos de 300 ms, así que los arrastres se ignoran.",
        "Gardez la souris immobile et cliquez normalement : une bonne souris indique 0 count. Seuls les clics faits "
        "après 100 ms sans mouvement et de moins de 300 ms sont mesurés, les glisser sont donc ignorés.",
        "Maus stillhalten und normal klicken: Eine gute Maus meldet 0 Counts. Gemessen werden nur Klicks nach "
        "100 ms ohne Bewegung und kürzer als 300 ms, Ziehen wird also ignoriert.",
        "Mantenha o mouse parado e clique normalmente: um bom mouse indica 0 counts. Só são medidos os cliques "
        "feitos após 100 ms sem movimento e com menos de 300 ms, então arrastos são ignorados.",
        "保持鼠标不动并正常点击：好的鼠标应为 0 counts。只测量静止 100 ms 后进行且短于 300 ms 的点击，因此拖动会被忽略。"},
    /* ResetButtons */ {"Reset buttons and wheel", "Azzera tasti e rotella", "Сбросить кнопки и колесо",
                        "Reiniciar botones y rueda", "Réinitialiser boutons et molette", "Tasten und Mausrad zurücksetzen",
                        "Reiniciar botões e roda", "重置按键和滚轮"},
};
static_assert(sizeof(kStrings) / sizeof(kStrings[0]) == size_t(Str::Count), "one row per Str id");

const char* kLangCodes[kLangs] = {"en", "it", "ru", "es", "fr", "de", "pt", "zh"};
const char* kLangNames[kLangs] = {"English", "Italiano", "Русский", "Español", "Français", "Deutsch", "Português", "中文（简体）"};

constexpr char kRegKey[] = "Software\\MouseTester";

} // namespace

const char* T(Str id) { return kStrings[int(id)][int(g_lang)]; }

Lang CurrentLanguage() { return g_lang; }

const char* LanguageName(Lang lang) { return kLangNames[int(lang)]; }

void SetLanguage(Lang lang) {
    g_lang = lang;
    // Remembered in the registry (current user only, no admin rights needed).
    const char* code = kLangCodes[int(lang)];
    RegSetKeyValueA(HKEY_CURRENT_USER, kRegKey, "Language", REG_SZ, code, DWORD(std::strlen(code) + 1));
}

void LoadLanguage() {
    char code[8] = {};
    DWORD size = sizeof(code);
    if (RegGetValueA(HKEY_CURRENT_USER, kRegKey, "Language", RRF_RT_REG_SZ, nullptr, code, &size) == ERROR_SUCCESS) {
        for (int i = 0; i < kLangs; ++i)
            if (std::strcmp(code, kLangCodes[i]) == 0) { g_lang = Lang(i); return; }
    }
    // First start: follow the Windows display language.
    switch (PRIMARYLANGID(GetUserDefaultUILanguage())) {
    case LANG_ITALIAN:    g_lang = Lang::It; break;
    case LANG_RUSSIAN:    g_lang = Lang::Ru; break;
    case LANG_SPANISH:    g_lang = Lang::Es; break;
    case LANG_FRENCH:     g_lang = Lang::Fr; break;
    case LANG_GERMAN:     g_lang = Lang::De; break;
    case LANG_PORTUGUESE: g_lang = Lang::Pt; break;
    case LANG_CHINESE:    g_lang = Lang::Zh; break;
    default:              g_lang = Lang::En; break;
    }
}

void AddTranslationGlyphs(ImFontGlyphRangesBuilder& builder) {
    for (const Row& row : kStrings)
        for (const char* s : row) builder.AddText(s);
    for (const char* s : kLangNames) builder.AddText(s);
}
