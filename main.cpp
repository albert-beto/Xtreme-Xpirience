#include <Arduino.h>
#include <avr/io.h>

/*
  Receptor:
  CH1 -> PD2 = directie
  CH2 -> PD4 = acceleratie

  TB6612:
  PWMA -> PD3 / OC2B
  AIN1 -> PD5
  AIN2 -> PD7

  PWMB -> PD6 / OC0A
  BIN1 -> PB0
  BIN2 -> PB1

*/

// serial debug

const unsigned long SERIAL_BAUD = 9600;
const unsigned long SERIAL_INTERVAL_MS = 250;
unsigned long lastSerialPrint = 0;

// led

#define LED_DDR   DDRB
#define LED_PORT  PORTB
#define LED_PIN   PB5

// receptor rc

#define CH1_PIN PD2
#define CH2_PIN PD4

// motor stang / Canal A

#define PWMA_PIN PD3
#define AIN1_PIN PD5
#define AIN2_PIN PD7

// motor drept / Canal B

#define PWMB_PIN PD6
#define BIN1_PIN PB0
#define BIN2_PIN PB1

// calibrari pentru mapare

const int CH1_MID = 1448;
const int CH2_MID = 1655;

const int RC_VALID_MIN = 850;
const int RC_VALID_MAX = 2350;

const int RC_MIN = 1000;
const int RC_MAX = 2100;

const int DEADZONE_US = 90;

// calibrari pentru motoare

const int MAX_SPEED = 180;
const int MIN_PWM = 115;

const int RAMP_STEP = 8;

const unsigned long TIMEOUT_US = 30000;

// inversari rapide

const bool INVERT_THROTTLE = false;
const bool INVERT_STEERING = false;

const bool INVERT_LEFT_MOTOR = false;
const bool INVERT_RIGHT_MOTOR = false;

// variabile globale

unsigned long lastBlink = 0;
bool ledState = false;

int currentLeft = 0;
int currentRight = 0;

// prototipuri functii

void init_gpio();
void init_pwm();

uint16_t read_rc_pulse(uint8_t pin);
int normalize_pulse(int valoare, int centru);
int limiteaza(int valoare, int minim, int maxim);
int aplica_minim_motor(int viteza);
int ramp_to(int current, int target);

void motor_stang(int viteza);
void motor_drept(int viteza);
void stop_motoare();

void update_led_miscare(bool miscare);

void afiseaza_semnale(
  uint16_t ch1,
  uint16_t ch2,
  int directie,
  int acceleratie,
  int targetLeft,
  int targetRight,
  int currentLeft,
  int currentRight,
  const char *status
);

// setup

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(500);

  Serial.println();
  Serial.println("=== START DEBUG RC ===");
  Serial.println("CH1 = directie | CH2 = acceleratie");
  Serial.println("Valori normale RC: aproximativ 1000 - 2000 us");
  Serial.println();

  init_gpio();
  init_pwm();

  stop_motoare();
  LED_PORT &= ~(1 << LED_PIN);
}

// loop

void loop() {
  uint16_t ch1 = read_rc_pulse(CH1_PIN);
  uint16_t ch2 = read_rc_pulse(CH2_PIN);

  if (ch1 == 0 || ch2 == 0) {
    currentLeft = ramp_to(currentLeft, 0);
    currentRight = ramp_to(currentRight, 0);

    motor_stang(currentLeft);
    motor_drept(currentRight);

    update_led_miscare(false);

    afiseaza_semnale(
      ch1,
      ch2,
      0,
      0,
      0,
      0,
      currentLeft,
      currentRight,
      "FARA SEMNAL"
    );

    return;
  }

  int directie = normalize_pulse(ch1, CH1_MID);
  int acceleratie = normalize_pulse(ch2, CH2_MID);

  if (INVERT_STEERING) {
    directie = -directie;
  }

  if (INVERT_THROTTLE) {
    acceleratie = -acceleratie;
  }

  bool manetaMiscata = (directie != 0 || acceleratie != 0);
  update_led_miscare(manetaMiscata);

  int targetLeft = 0;
  int targetRight = 0;

  if (acceleratie == 0 && directie != 0) {
    targetLeft = directie;
    targetRight = -directie;
  } else {
    targetLeft = acceleratie + directie;
    targetRight = acceleratie - directie;
  }

  targetLeft = limiteaza(targetLeft, -MAX_SPEED, MAX_SPEED);
  targetRight = limiteaza(targetRight, -MAX_SPEED, MAX_SPEED);

  targetLeft = aplica_minim_motor(targetLeft);
  targetRight = aplica_minim_motor(targetRight);

  currentLeft = ramp_to(currentLeft, targetLeft);
  currentRight = ramp_to(currentRight, targetRight);

  motor_stang(currentLeft);
  motor_drept(currentRight);

  afiseaza_semnale(
    ch1,
    ch2,
    directie,
    acceleratie,
    targetLeft,
    targetRight,
    currentLeft,
    currentRight,
    manetaMiscata ? "MISCARE" : "CENTRU"
  );
}

// afisare terminal serial

void afiseaza_semnale(
  uint16_t ch1,
  uint16_t ch2,
  int directie,
  int acceleratie,
  int targetLeft,
  int targetRight,
  int currentLeft,
  int currentRight,
  const char *status
) {
  if (millis() - lastSerialPrint < SERIAL_INTERVAL_MS) {
    return;
  }

  lastSerialPrint = millis();

  Serial.print("CH1=");
  Serial.print(ch1);
  Serial.print(" us");

  Serial.print(" | CH2=");
  Serial.print(ch2);
  Serial.print(" us");

  Serial.print(" | directie=");
  Serial.print(directie);

  Serial.print(" | acceleratie=");
  Serial.print(acceleratie);

  Serial.print(" | targetL=");
  Serial.print(targetLeft);

  Serial.print(" | targetR=");
  Serial.print(targetRight);

  Serial.print(" | pwmL=");
  Serial.print(currentLeft);

  Serial.print(" | pwmR=");
  Serial.print(currentRight);

  Serial.print(" | status=");
  Serial.println(status);
}

// initializare GPIO 

void init_gpio() {
  LED_DDR |= (1 << LED_PIN);

  // Receptor: PD2 si PD4 input
  DDRD &= ~(1 << CH1_PIN);
  DDRD &= ~(1 << CH2_PIN);

  // Fara pull-up intern pe receptor
  PORTD &= ~(1 << CH1_PIN);
  PORTD &= ~(1 << CH2_PIN);

  // Motor stang pe PORTD
  DDRD |= (1 << PWMA_PIN);
  DDRD |= (1 << AIN1_PIN);
  DDRD |= (1 << AIN2_PIN);

  // PWM motor drept pe PD6
  DDRD |= (1 << PWMB_PIN);

  // Directii motor drept mutate pe PORTB
  DDRB |= (1 << BIN1_PIN);
  DDRB |= (1 << BIN2_PIN);

  // Initial oprim directiile motorului drept
  PORTB &= ~(1 << BIN1_PIN);
  PORTB &= ~(1 << BIN2_PIN);
}

// initializare PWM

void init_pwm() {
    // PWMA pe PD3 = OC2B
    // Timer2 Fast PWM
  

  TCCR2A = 0;
  TCCR2B = 0;

  TCCR2A |= (1 << WGM20) | (1 << WGM21);
  TCCR2A |= (1 << COM2B1);

  // Prescaler 64
  TCCR2B |= (1 << CS22);

  OCR2B = 0;

    // PWMB pe PD6 = OC0A
    // Timer0 Fast PWM

  TCCR0A |= (1 << WGM00) | (1 << WGM01);
  TCCR0A |= (1 << COM0A1);

  // Prescaler 64
  TCCR0B |= (1 << CS01) | (1 << CS00);

  OCR0A = 0;
}

// citire receptor RC

uint16_t read_rc_pulse(uint8_t pin) {
  unsigned long startWait = micros();

  while (!(PIND & (1 << pin))) {
    if (micros() - startWait > TIMEOUT_US) {
      return 0;
    }
  }

  unsigned long startPulse = micros();

  while (PIND & (1 << pin)) {
    if (micros() - startPulse > TIMEOUT_US) {
      return 0;
    }
  }

  unsigned long durata = micros() - startPulse;

  if (durata < RC_VALID_MIN || durata > RC_VALID_MAX) {
    return 0;
  }

  return (uint16_t)durata;
}

// mapare si normalizare semnale RC

int normalize_pulse(int valoare, int centru) {
  int diferenta = valoare - centru;

  if (diferenta > -DEADZONE_US && diferenta < DEADZONE_US) {
    return 0;
  }

  valoare = limiteaza(valoare, RC_MIN, RC_MAX);

  int rezultat;

  if (valoare > centru + DEADZONE_US) {
    rezultat = map(valoare, centru + DEADZONE_US, RC_MAX, 0, MAX_SPEED);
  } else {
    rezultat = map(valoare, RC_MIN, centru - DEADZONE_US, -MAX_SPEED, 0);
  }

  return limiteaza(rezultat, -MAX_SPEED, MAX_SPEED);
}

// functii auxiliare

int limiteaza(int valoare, int minim, int maxim) {
  if (valoare < minim) {
    return minim;
  }

  if (valoare > maxim) {
    return maxim;
  }

  return valoare;
}

int aplica_minim_motor(int viteza) {
  if (viteza == 0) {
    return 0;
  }

  if (viteza > 0 && viteza < MIN_PWM) {
    return MIN_PWM;
  }

  if (viteza < 0 && viteza > -MIN_PWM) {
    return -MIN_PWM;
  }

  return viteza;
}

int ramp_to(int current, int target) {
  if (current < target) {
    current += RAMP_STEP;

    if (current > target) {
      current = target;
    }
  } else if (current > target) {
    current -= RAMP_STEP;

    if (current < target) {
      current = target;
    }
  }

  return current;
}

// conttrol motor stang / Canal A

void motor_stang(int viteza) {
  if (INVERT_LEFT_MOTOR) {
    viteza = -viteza;
  }

  if (viteza > 0) {
    PORTD |= (1 << AIN1_PIN);
    PORTD &= ~(1 << AIN2_PIN);
    OCR2B = (uint8_t)viteza;
  } else if (viteza < 0) {
    PORTD &= ~(1 << AIN1_PIN);
    PORTD |= (1 << AIN2_PIN);
    OCR2B = (uint8_t)(-viteza);
  } else {
    PORTD &= ~(1 << AIN1_PIN);
    PORTD &= ~(1 << AIN2_PIN);
    OCR2B = 0;
  }
}

// control motor drept / Canal B

void motor_drept(int viteza) {
  if (INVERT_RIGHT_MOTOR) {
    viteza = -viteza;
  }

  if (viteza > 0) {
    PORTB |= (1 << BIN1_PIN);
    PORTB &= ~(1 << BIN2_PIN);
    OCR0A = (uint8_t)viteza;
  } else if (viteza < 0) {
    PORTB &= ~(1 << BIN1_PIN);
    PORTB |= (1 << BIN2_PIN);
    OCR0A = (uint8_t)(-viteza);
  } else {
    PORTB &= ~(1 << BIN1_PIN);
    PORTB &= ~(1 << BIN2_PIN);
    OCR0A = 0;
  }
}

// stop motoare

void stop_motoare() {
  OCR2B = 0;
  OCR0A = 0;

  PORTD &= ~(1 << AIN1_PIN);
  PORTD &= ~(1 << AIN2_PIN);

  PORTB &= ~(1 << BIN1_PIN);
  PORTB &= ~(1 << BIN2_PIN);
}

// led

void update_led_miscare(bool miscare) {
  if (!miscare) {
    LED_PORT &= ~(1 << LED_PIN);
    ledState = false;
    return;
  }

  if (millis() - lastBlink >= 250) {
    lastBlink = millis();
    ledState = !ledState;

    if (ledState) {
      LED_PORT |= (1 << LED_PIN);
    } else {
      LED_PORT &= ~(1 << LED_PIN);
    }
  }
}
