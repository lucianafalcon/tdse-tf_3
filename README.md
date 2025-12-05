# Interfaz EMG para Monitoreo de Actividad Muscular

### Autora: **Luciana Falcon**  
### Padrón: **107316**  

---

## Descripción  

El proyecto consiste en el desarrollo de un **sistema embebido capaz de adquirir, procesar y visualizar señales bioeléctricas musculares (EMG)** mediante **electrodos de registro no invasivos**.  

El sistema permite la **detección de contracciones musculares** a través de procesamiento digital en el microcontrolador **STM32**, mostrando los resultados en un **display local** y enviando los datos por **Bluetooth** a una PC o dispositivo móvil para análisis o visualización.  

El objetivo principal es **diseñar un sistema embebido completo** que integre todas las etapas del procesamiento biológico:
- **Sensado analógico** (electrodos + amplificador).  
- **Acondicionamiento y digitalización** (ADC del microcontrolador).  
- **Procesamiento y detección de eventos** (software embebido).  
- **Comunicación y visualización** (Bluetooth + display).  

---

## Alcance del Proyecto  

- Adquisición de señales EMG con electrodos no invasivos y amplificación mediante módulo AD8232.  
- Detección de contracciones musculares mediante procesamiento digital y detección de umbrales.  
- Visualización en tiempo real del nivel de activación muscular en un display OLED.  
- Transmisión de datos a través de Bluetooth hacia una PC o dispositivo móvil.  
- Activación de un LED o alerta sonora al superar el umbral de contracción.  

---

## Requerimientos  

### Plataforma de desarrollo  
- **Placa utilizada:** NUCLEO-F103RB  
- **Microcontrolador:** STM32 

### Firmware  
Implementación tipo **Super Loop (bare-metal, event-triggered)** con tareas periódicas:
- Lectura ADC / I2C.  
- Procesamiento digital y detección de picos.  
- Comunicación UART–BLE (Bluetooth).  
- Actualización de display OLED.  

---

### Hardware  

- **Dip Switch / Botón:** permite iniciar o detener la lectura de señales EMG.  
- **Buzzer:** emite señal sonora cuando el usuario supera el umbral de contracción.  
- **Display OLED (I2C):** muestra el nivel de contracción en tiempo real.  
- **Módulo Bluetooth HM-10:** transmite los datos EMG hacia la PC o dispositivo móvil.  
- **Memoria EEPROM externa / Flash interna:** almacena los datos históricos o parámetros de calibración.  
- **Sensor analógico (AD8232 + electrodos):** capta señales musculares (EMG) en el rango de milivoltios y las entrega al ADC del STM32.  

---

## Diagrama en Bloques  

<img width="1243" height="828" alt="embebidos" src="https://github.com/user-attachments/assets/35658773-ff54-48d2-b060-35d2d7419d01" />

---

## Consumo y factor de carga

En la Tabla 1.1 se presenta el análisis de consumo del sistema bajo distintos escenarios de funcionamiento. Las mediciones fueron realizadas alimentando el nodo desde una fuente regulada a 5 V,
utilizando un multímetro en serie para registrar la corriente consumida por el STM32 y sus periféricos en cada caso. Cada escenario se midió durante varias ejecuciones para obtener un valor representativo.

| Escenario              | Periféricos activos                      | Descripción de la prueba                                                              | Consumo (mA) |
|------------------------|-------------------------------------------|--------------------------------------------------------------------------------------|--------------|
| **Baseline**           | Ninguno                                   | Microcontrolador ejecutando loop mínimo, sin periféricos habilitados.                |    12,9      |
| **ADC activo**         | ADC1                                      | Conversión continua de señal EMG proveniente del AD8232.                             |    14,1      |
| **I2C activo**         | I2C1 (OLED + EEPROM)                      | Comunicación con OLED y acceso a memoria I2C, refresco de pantalla.                  |    13,4      |
| **UART activo**        | USART1 (BLE HM-10)                        | Transmisión periódica de nivel de contracción al módulo Bluetooth HM-10.             |    11,4      |
| **Buzzer**             | GPIO                                      | Activación del buzzer cuando el nivel de contracción supera el umbral configurado.   |    13,2      |
| **Super Loop completo**| ADC + I2C + UART + GPIO                   | Funcionamiento total: adquisición EMG, procesamiento, transmisión BLE y OLED activo. |    14,5      |
<p align="center"><b>Tabla 1.1 — Escenarios de medición de consumo.</b></p>

En la Tabla 1.2 se enumeran las tareas periódicas presentes en el super-loop y se estimaron sus tiempos de ejecución mediante mediciones instrumentadas con el temporizador
interno del STM32, junto con su período correspondiente.

| Tarea                   | Descripción                         | Tiempo de CPU (Ci) | Período (Ti) | Ci/Ti     |
|-------------------------|--------------------------------------|---------------------|--------------|-----------|
| **Adquisición EMG (ADC)**     | Muestreo continuo a 1 kHz              | 0,02 ms             | 1 ms         | 0,020     |
| **Procesamiento EMG**         | Filtrado, RMS y cálculo del nivel      | 0,40 ms             | 20 ms        | 0,020     |
| **Display OLED (I2C)**        | Refresco de pantalla                   | 1,50 ms             | 100 ms       | 0,015     |
| **Bluetooth BLE (UART)**      | Envío del valor procesado              | 0,30 ms             | 20 ms        | 0,015     |
| **Detección de umbral**       | Comparación y activación del buzzer    | 0,05 ms             | 20 ms        | 0,0025    |
| **Lectura de botón**          | Lectura periódica de entrada digital   | 0,02 ms             | 50 ms        | 0,0004    |

<p align="center"><b>Tabla 1.2 — Tareas periódicas consideradas para el factor de carga.</b></p>

**Factor de uso total del sistema:**  
En base a los tiempos de ejecución y periodos definidos en la Tabla 1.2, se calcula el factor de uso total del sistema como la suma de Ci/Ti de todas las tareas periódicas.

u = 0.020 + 0.020 + 0.015 + 0.015 + 0.0025 + 0.0004 = **0.0729 → 7.29%**

---

## Elicitación de Requisitos y Casos de Uso

La Tabla 2.1 resume las funciones esenciales que el sistema debe llevar a cabo para cumplir con los objetivos del proyecto. 
Cada requisito se codifica con un identificador único para permitir su trazabilidad en el diseño y la implementación.
| Código | Requisito Funcional |
|--------|----------------------|
| **RF1** | El sistema debe adquirir la señal EMG mediante electrodos conectados al módulo AD8232. |
| **RF2** | El sistema debe digitalizar la señal con el ADC del STM32 a al menos 1 kHz. |
| **RF3** | El sistema debe procesar la señal (RMS, filtrado, umbral). |
| **RF4** | El sistema debe mostrar el nivel de actividad muscular en el display OLED. |
| **RF5** | El sistema debe transmitir los datos procesados por Bluetooth hacia una PC o dispositivo móvil. |
| **RF6** | El sistema debe activar un buzzer cuando la actividad muscular supere un umbral configurable. |
| **RF7** | El sistema debe permitir iniciar/detener el monitoreo mediante un botón. |
<p align="center"><b>Tabla 2.1 — Requisitos Funcionales RF.</b></p>

La Tabla 2.2 presenta las restricciones de desempeño y las condiciones operativas que debe cumplir el sistema. Estos requisitos garantizan eficiencia, 
respuesta temporal adecuada y estabilidad en las comunicaciones.
| Código | Requisito No Funcional |
|--------|-------------------------|
| **RNF1** | El sistema debe mantener un consumo menor a 20 mA en operación estándar. |
| **RNF2** | La interfaz Bluetooth debe asegurar comunicación continua. |
| **RNF3** | El display debe actualizarse al menos cada 100 ms. |
| **RNF4** | La detección de contracción debe ocurrir con una latencia menor a 50 ms. |
| **RNF5** | El firmware debe estar implementado en arquitectura super-loop. |
<p align="center"><b>Tabla 2.2 — Requisitos No Funcionales RNF.</b></p>

En las tablas 3.1 a 4.2 se presentan los 2 casos de uso para el sistema.

| Ítem | Descripción |
|------|-------------|
| **Actores** | Usuario, Sistema EMG. |
| **Precondiciones** | El dispositivo está encendido y los electrodos colocados. |
<p align="center"><b>Tabla 3.1 — Caso de Uso 1: Monitoreo de Actividad Muscular.</b></p>

| Paso | Acción |
|------|--------|
| **1** | El usuario presiona el botón para iniciar el monitoreo. |
| **2** | El sistema comienza a muestrear la señal EMG. |
| **3** | Se procesa la señal y se calcula el nivel de activación muscular. |
| **4** | El valor procesado se envía al display y por Bluetooth. |
| **5** | Si el nivel supera el umbral, se activa el buzzer. |
<p align="center"><b>Tabla 3.2 — Flujo Principal.</b></p>

| Paso | Acción |
|------|--------|
| **A1** | El usuario presiona nuevamente el botón y el sistema detiene la adquisición. |
<p align="center"><b>Tabla 3.3 — Flujo Alternativo.</b></p>

| Ítem | Descripción |
|------|-------------|
| **Actor** | Sistema EMG. |
| **Precondiciones** | El sistema está en funcionamiento y la señal EMG está siendo adquirida. |
| **Postcondiciones** | El buzzer se activa o desactiva según el nivel de activación muscular. |
| **Descripción general** | El sistema analiza continuamente el nivel RMS de la señal EMG y genera una alarma cuando se supera un umbral establecido. |
<p align="center"><b>Tabla 4.1 – Caso de Uso 2: Alarma por Umbral de Activación.</b></p>

| Paso | Acción |
|------|--------|
| **1** | El sistema calcula el RMS de la señal EMG en cada ventana de procesamiento. |
| **2** | Compara el nivel con el umbral configurado. |
| **3** | Si lo supera, activa el buzzer. |
| **4** | Cuando el valor RMS vuelve por debajo del umbral, el sistema desactiva el buzzer. |
<p align="center"><b>Tabla 4.2 — Flujo.</b></p>


