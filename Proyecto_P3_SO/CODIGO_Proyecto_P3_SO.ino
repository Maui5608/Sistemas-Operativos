// ======================================================
// CONFIGURACIÓN DE PINES
// ======================================================
const int btnInterrupcion = 2;   // Botón que agrega A/B/C según prioridad
const int btnPrioridad    = 3;   // Cambia prioridad (0=C, 1=B, 2=A)
const int btnBuffer       = 4;   // Envía el buffer

const int ledRojo     = 5;
const int ledAmarillo = 6;
const int ledVerde    = 7;

// ======================================================
// BUFFER CIRCULAR
// ======================================================
char buffer[16];
int head = 0;
int tail = 0;
int count = 0;

// 0 = C, 1 = B, 2 = A
volatile int prioridad = 0;

volatile bool prioridadCambio = false;
volatile unsigned long lastInterruptTime = 0;  // Antirrebote real


// ======================================================
// AGREGAR LETRA AL BUFFER
// ======================================================
void agregarBuffer(char tecla) {
  if (count < 16) {
    buffer[tail] = tecla;
    tail = (tail + 1) % 16;
    count++;
  }
}


// ======================================================
// ENVIAR EL CONTENIDO DEL BUFFER
// ======================================================
void enviarBuffer() {
  Serial.println("\n=== Enviando contenido del buffer ===");

  if (count == 0) {
    Serial.println("=== Buffer vacío ===");
    return;
  }

  while (count > 0) {
    char tecla = buffer[head];
    head = (head + 1) % 16;
    count--;

    Serial.print("Tecla enviada: ");
    Serial.println(tecla);

    delay(200);
  }

  Serial.println("=== Buffer vacío ===");
}


// ======================================================
// ACTUALIZAR LEDs SEGÚN OCUPACIÓN DEL BUFFER
// ======================================================
void actualizarLEDs() {
  if (count < 6) {          // 0–5
    digitalWrite(ledVerde, HIGH);
    digitalWrite(ledAmarillo, LOW);
    digitalWrite(ledRojo, LOW);
  }
  else if (count < 12) {    // 6–11
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarillo, HIGH);
    digitalWrite(ledRojo, LOW);
  }
  else {                    // 12–15
    digitalWrite(ledVerde, LOW);
    digitalWrite(ledAmarillo, LOW);
    digitalWrite(ledRojo, HIGH);
  }
}


// ======================================================
// INTERRUPCIÓN: AGREGAR LETRA
// ======================================================
void ISR_interrupcion() {
  unsigned long t = millis();

  // antirrebote
  if (t - lastInterruptTime < 200) return;
  lastInterruptTime = t;

  char tecla;

  if (prioridad == 2) tecla = 'A';   // Prioridad alta
  else if (prioridad == 1) tecla = 'B';
  else tecla = 'C';                  // Prioridad baja

  agregarBuffer(tecla);
}


// ======================================================
// INTERRUPCIÓN: CAMBIAR PRIORIDAD
// ======================================================
void ISR_prioridad() {
  prioridad = (prioridad + 1) % 3;
  prioridadCambio = true;
}


// ======================================================
// SETUP
// ======================================================
void setup() {
  Serial.begin(9600);

  pinMode(btnInterrupcion, INPUT_PULLUP);
  pinMode(btnPrioridad, INPUT_PULLUP);
  pinMode(btnBuffer, INPUT_PULLUP);

  pinMode(ledRojo, OUTPUT);
  pinMode(ledAmarillo, OUTPUT);
  pinMode(ledVerde, OUTPUT);

  attachInterrupt(digitalPinToInterrupt(btnInterrupcion), ISR_interrupcion, FALLING);
  attachInterrupt(digitalPinToInterrupt(btnPrioridad), ISR_prioridad, FALLING);

  Serial.println("Sistema iniciado...\n");
}


// ======================================================
// LOOP PRINCIPAL
// ======================================================
void loop() {

  // Mostrar prioridad cuando cambie
  if (prioridadCambio) {
    prioridadCambio = false;

    Serial.print("Nueva prioridad: ");
    Serial.println(prioridad);
  }

  // Botón que envía el buffer (D4)
  if (digitalRead(btnBuffer) == LOW) {
    enviarBuffer();
    delay(300);
  }

  // Actualizar LEDs
  actualizarLEDs();

  delay(40);
}
