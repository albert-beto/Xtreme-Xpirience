===== README - Firmware Masinuta RC =====

Acest cod controleaza o masinuta RC folosind placa ATmega328P Xplained Mini, un receptor RC si driverul de motoare TB6612FNG.

Programul citeste doua semnale de la receptor:

  * CH1 - directie
  * CH2 - acceleratie / mers inainte si inapoi

Pe baza acestor semnale, microcontrollerul controleaza separat cele doua motoare ale masinutei.

==== Pini folositi ====

^ Semnal ^ Pin ATmega328P ^ Rol ^
| CH1 | PD2 | Directie |
| CH2 | PD4 | Acceleratie |
| PWMA | PD3 | PWM motor stang |
| AIN1 | PD5 | Sens motor stang |
| AIN2 | PD7 | Sens motor stang |
| PWMB | PD6 | PWM motor drept |
| BIN1 | PD0 | Sens motor drept |
| BIN2 | PD1 | Sens motor drept |
| LED onboard | PB5 | Semnalizare |

==== Functionalitati ====

  * citirea semnalelor PWM de la receptorul RC;
  * controlul vitezei motoarelor prin PWM;
  * controlul sensului de rotatie prin TB6612FNG;
  * mers inainte, inapoi, stanga si dreapta;
  * oprirea motoarelor daca semnalul RC lipseste;
  * semnalizare prin LED-ul onboard.

==== Alimentare ====

Partea logica functioneaza la 5V:

  * ATmega328P Xplained Mini
  * receptor RC
  * VCC si STBY de la TB6612FNG

Motoarele sunt alimentate separat:

  * +6V acumulator NiMH -> VM TB6612FNG
  * - acumulator NiMH -> GND comun

Este important ca toate modulele sa aiba GND comun.

==== Observatii ====

Codul nu foloseste Serial Monitor, deoarece pinii PD0 si PD1 sunt folositi pentru controlul motorului drept.
