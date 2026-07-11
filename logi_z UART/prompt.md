
Pracuję nad sterowaniem modelarskiej „armatki” na STM32 z dwoma osiami (OS1, OS2) i zabezpieczeniem przeciążeniowym na podstawie pomiaru prądu przez INA3221. Silniki są sterowane pinami: OS1 (GORA/DOL), OS2 (CCW/CW). W projekcie mam m.in. pliki main.c i silniki.c.

Wczoraj w innym wątku analizowałem z asystentem logi z kalibracji prądowej i homingu. Kluczowe informacje z tamtej sesji:

Progi prądowe po kalibracji:

thresh_ch1_mA = 60.0 mA (CH1 – OS2, CW/CCW),

thresh_ch2_mA = 45.0 mA (CH2 – OS1, GORA/DOL).

W silniki.c mam funkcję jedz_do_zderzaka_blok(uint16_t pin, uint8_t ch), która wykrywa zderzak na podstawie prądu:

pobiera prąd z INA3221: I_mA = fabsf(INA3221_GetCurrentA(ch) * 1000.0f);

liczy przyrost: dI = I_mA - I_prev;

dotychczasowy warunek:

c
uint8_t condI  = (I_mA > thresh);
uint8_t condDI = (dI > 9.4f && I_mA > thresh * 0.5f);
uint8_t hit    = condI || condDI;
gdy hit powtarza się STALL_CONFIRM_COUNT razy (u mnie 1), uznajemy „zderzak”.

Dodałem logowanie przez UART:

linie [CUR] z I, dI, thr, cnt,

linie [DBG] hit=1 condI=... condDI=...,

komunikaty STALL?, ZDERZAK! itd.

Z analizy konkretnych logów wyszły takie obserwacje:

Przykłady prawdopodobnych realnych zderzaków (prawdziwy stall):

CH2 (OS1, GORA/DOL):

I rząd 110–125 mA przy progu 45 mA, np.:

text
[CUR] ... I=81.6 → 92.4 → 111.6 mA ...
[DBG] hit=1 condI=1 condDI=0 CH2 I=111.6 dI=8.8 thr=45.0
CH1 (OS2, CW/CCW):

I rzędu 100+ mA przy progu 60 mA, np.:

text
[DBG] hit=1 condI=1 condDI=0 CH1 I=104.8 dI=2.8 thr=60.0
W tych przypadkach stall jest wyzwalany przez condI (prąd > próg), co wygląda sensownie.

Przykłady ewidentnych fałszywych stallów (fałszywe zadziałanie algorytmu, mechanicznie NIE dojechał do zderzaka – oceniane „na ucho” i z obserwacji):

CH2:

text
[CUR] ... I≈18–25 mA, thr=45.0 ...
[DBG] hit=1 condI=0 condDI=1 CH2 I=24.8 dI≈9.6 thr=45.0
...
[CUR] ... ZDERZAK! CH2 I=24.8 mA ...
CH2 kolejny przypadek:

text
[DBG] hit=1 condI=0 condDI=1 CH2 I=32.8 dI≈9.6 thr=45.0
CH1:

text
[DBG] hit=1 condI=0 condDI=1 CH1 I=40.8 dI≈10.8 thr=60.0
We wszystkich tych fałszywych stallach:

I_mA jest sporo poniżej progu thresh,

condI=0,

condDI=1 – stall wynika wyłącznie ze skoku dI.

Zaczęliśmy też ręcznie oznaczać fałszywe stalle w logu, wciskając f w terminalu (TIO na Raspberry Pi). Widać to jako:

text
f[CUR] t=...
– docelowo używam tego jako znaczników w logu: wszystko tuż przed takim f traktuję jako FAKE STALL.

Moje wnioski z wczoraj:

Główny problem to za agresywna detekcja na podstawie samej pochodnej prądu (dI), bo generuje fałszywe stalle przy normalnych prądach roboczych (20–40 mA), kiedy mechanika jeszcze nie dojechała do zderzaka.

Detekcja na samym I > thresh (condI) wydaje się sensowna, bo przy realnym zderzaku prąd wyraźnie przekracza próg (czasem 2× i więcej).

PROŚBA NA DZIŚ:

Na podstawie powyższego kontekstu i przykładowych logów zaproponuj mi nową, bezpieczniejszą formułę hit w jedz_do_zderzaka_blok(), tak żeby:

w pierwszym kroku można było całkowicie wyłączyć condDI (pochodną) lub mocno ją złagodzić,

ochronę przekładni nadal zapewniał próg prądu (condI),

można było później, po zebraniu kolejnych logów, ewentualnie z powrotem dodać condDI, ale już ze znacznie ostrzejszymi warunkami (np. większy próg dI, oraz I_mA blisko progu, a nie 0.5 * thr).

Pokaż mi, proszę, konkretny fragment kodu C do wklejenia w silniki.c, w stylu:

c
uint8_t condI  = ...
uint8_t condDI = ...
uint8_t hit    = ...
– w dwóch wariantach:

Wariant A: pochodna całkowicie wyłączona (tylko condI).

Wariant B: pochodna „złagodzona”, np. coś typu: dI > X i I_mA > Y% progu, z parametrami dobranymi pod moje zakresy prądów z logów.

Jeśli uznasz, że warto, zaproponuj też minimalne zmiany w jedz_przez_czas(), żeby logika stall detection była z nim spójna, ale priorytet ma na razie jedz_do_zderzaka_blok().

Pamiętaj, że mechanicznie:

realny zderzak słychać i w logach widać wyraźny wzrost prądu powyżej progu,

fałszywe stalle widziałem przy I znacznie poniżej progu, wyzwalane tylko przez dI.

Chciałbym z Tobą kontynuować tak, jakbyś znał ten kontekst z poprzedniego wątku.

