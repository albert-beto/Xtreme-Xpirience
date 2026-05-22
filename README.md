# RC Car Firmware

Firmware pentru o mașinuță RC controlată cu o placă **ATmega328P Xplained Mini**, un receptor RC și un driver de motoare **TB6612FNG**.

## Descriere

Programul citește semnalele PWM primite de la receptorul RC și controlează separat cele două motoare ale mașinuței.  
Canalul `CH1` este folosit pentru direcție, iar `CH2` pentru accelerație și mers înainte/înapoi.

Controlul motoarelor este realizat prin driverul TB6612FNG, folosind pini digitali pentru sens și semnale PWM pentru viteză.

## Pini folosiți

| Semnal | Pin ATmega328P | Descriere |
|---|---|---|
| CH1 | PD2 | Direcție receptor RC |
| CH2 | PD4 | Accelerație receptor RC |
| PWMA | PD3 | PWM motor stâng |
| AIN1 | PD5 | Sens motor stâng |
| AIN2 | PD7 | Sens motor stâng |
| PWMB | PD6 | PWM motor drept |
| BIN1 | PD0 | Sens motor drept |
| BIN2 | PD1 | Sens motor drept |
| LED onboard | PB5 | Semnalizare stare |

## Funcționalități

- citirea semnalelor PWM de la receptorul RC;
- controlul vitezei motoarelor prin PWM;
- controlul sensului de rotație prin TB6612FNG;
- mers înainte, înapoi, stânga și dreapta;
- oprirea motoarelor dacă semnalul RC lipsește;
- semnalizare prin LED-ul onboard.

## Alimentare

Partea logică funcționează la **5V**:

- ATmega328P Xplained Mini;
- receptor RC;
- `VCC` și `STBY` de la TB6612FNG.

Motoarele sunt alimentate separat:

```text
+6V acumulator NiMH -> VM TB6612FNG
- acumulator NiMH  -> GND comun
