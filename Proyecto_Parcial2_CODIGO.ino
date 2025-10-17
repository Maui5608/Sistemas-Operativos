#include <Arduino_FreeRTOS.h>
#include <semphr.h>

// ==================== Pines (según proyecto) ====================
#define LED_PROD_ROJO     2   // productor: buffer lleno
#define LED_PROD_AMARILLO 3   // productor: intentando acceder al buffer
#define LED_PROD_VERDE    4   // productor: produciendo

#define LED_CONS_ROJO     5   // consumidor: buffer vacío
#define LED_CONS_AMARILLO 6   // consumidor: intentando acceder al buffer
#define LED_CONS_VERDE    7   // consumidor: consumiendo

// ==================== Buffer circular ====================
#define BUFFER_SIZE 5
volatile int buffer[BUFFER_SIZE];
volatile int inIndex  = 0;   // inserta productor
volatile int outIndex = 0;   // extrae consumidor

// ==================== Semáforos ====================
SemaphoreHandle_t mutex;               // exclusión mutua (región crítica)
SemaphoreHandle_t espaciosDisponibles; // cuenta espacios libres (vacias)
SemaphoreHandle_t itemsDisponibles;    // cuenta items listos (llenas)

// (opcional) mutex para serial, evita prints entrecortados
SemaphoreHandle_t serialMutex;

// ==================== Utilidades ====================
static inline void setLed(int pin, bool on) { digitalWrite(pin, on ? HIGH : LOW); }

static inline void printBufferXde5() {
  UBaseType_t x = uxSemaphoreGetCount(itemsDisponibles); // items actuales en buffer
  if (xSemaphoreTake(serialMutex, portMAX_DELAY) == pdTRUE) {
    Serial.print("Buffer ");
    Serial.print((int)x);
    Serial.println("/5");
    xSemaphoreGive(serialMutex);
  }
}

static inline int producir_elemento() { return random(0, 100); }
static inline void insertar_elemento(int e) { buffer[inIndex] = e; inIndex = (inIndex + 1) % BUFFER_SIZE; }
static inline int  quitar_elemento() { int x = buffer[outIndex]; outIndex = (outIndex + 1) % BUFFER_SIZE; return x; }
static inline void consumir_elemento(int e) { (void)e; /* aquí harías algo si quieres */ }

// ==================== Tarea: Productor ====================
void TaskProductor(void *pv) {
  (void)pv;
  for (;;) {
    // Genera número aleatorio (0..99)
    int elemento = producir_elemento();

    // Amarillo: quiere acceder al buffer
    setLed(LED_PROD_AMARILLO, true);

    // Rojo si el buffer está lleno (espacios = 0) —indicador previo
    if (uxSemaphoreGetCount(espaciosDisponibles) == 0) {
      setLed(LED_PROD_ROJO, true);
    } else {
      setLed(LED_PROD_ROJO, false);
    }

    // Espera a que haya un espacio disponible
    xSemaphoreTake(espaciosDisponibles, portMAX_DELAY);

    // Toma el mutex (región crítica)
    xSemaphoreTake(mutex, portMAX_DELAY);

      // Verde: produciendo
      setLed(LED_PROD_VERDE, true);
      insertar_elemento(elemento);
      // Pequeña pausa visual para notar el verde
      vTaskDelay(120 / portTICK_PERIOD_MS);
      setLed(LED_PROD_VERDE, false);

    // Libera el mutex
    xSemaphoreGive(mutex);

    // Incrementa ítems disponibles
    xSemaphoreGive(itemsDisponibles);

    // Ya no está “intentando”
    setLed(LED_PROD_AMARILLO, false);
    // Si se iluminó rojo por lleno, apágalo al terminar
    setLed(LED_PROD_ROJO, false);

    // Solo imprimir Buffer X/5
    printBufferXde5();

    // Ritmo moderado (no requerido, solo para que se aprecie el comportamiento)
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// ==================== Tarea: Consumidor ====================
void TaskConsumidor(void *pv) {
  (void)pv;
  for (;;) {
    // Amarillo: quiere acceder al buffer
    setLed(LED_CONS_AMARILLO, true);

    // Rojo si el buffer está vacío (items = 0) —indicador previo
    if (uxSemaphoreGetCount(itemsDisponibles) == 0) {
      setLed(LED_CONS_ROJO, true);
    } else {
      setLed(LED_CONS_ROJO, false);
    }

    // Espera a que haya un ítem disponible
    xSemaphoreTake(itemsDisponibles, portMAX_DELAY);

    // Toma el mutex (región crítica)
    xSemaphoreTake(mutex, portMAX_DELAY);

      // Verde: consumiendo
      setLed(LED_CONS_VERDE, true);
      int elemento = quitar_elemento();
      (void)elemento; // no pediste imprimir el valor, solo el estado del buffer
      vTaskDelay(120 / portTICK_PERIOD_MS);
      setLed(LED_CONS_VERDE, false);

    // Libera el mutex
    xSemaphoreGive(mutex);

    // Incrementa espacios disponibles
    xSemaphoreGive(espaciosDisponibles);

    // Ya no está “intentando”
    setLed(LED_CONS_AMARILLO, false);
    // Apaga rojo si estaba encendido por vacío
    setLed(LED_CONS_ROJO, false);

    // Solo imprimir Buffer X/5
    printBufferXde5();

    // Ritmo moderado
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

// ==================== Setup / Loop ====================
void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0));

  pinMode(LED_PROD_ROJO, OUTPUT);
  pinMode(LED_PROD_AMARILLO, OUTPUT);
  pinMode(LED_PROD_VERDE, OUTPUT);
  pinMode(LED_CONS_ROJO, OUTPUT);
  pinMode(LED_CONS_AMARILLO, OUTPUT);
  pinMode(LED_CONS_VERDE, OUTPUT);

  // Semáforos del proyecto: mutex, espacios, items
  mutex             = xSemaphoreCreateMutex();
  espaciosDisponibles = xSemaphoreCreateCounting(BUFFER_SIZE, BUFFER_SIZE); // buffer vacío al inicio
  itemsDisponibles    = xSemaphoreCreateCounting(BUFFER_SIZE, 0);
  serialMutex         = xSemaphoreCreateMutex();

  // Tareas
  xTaskCreate(TaskProductor,  "Productor", 256, NULL, 1, NULL);
  xTaskCreate(TaskConsumidor, "Consumidor", 256, NULL, 1, NULL);
}

void loop() {
  // FreeRTOS maneja las tareas
}
