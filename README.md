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

| Escenario              | Periféricos activos                      | Descripción de la prueba                                                              | Consumo (mA) |
|------------------------|-------------------------------------------|--------------------------------------------------------------------------------------|--------------|
| **Baseline**           | Ninguno                                   | Microcontrolador ejecutando loop mínimo, sin periféricos habilitados.                |    12.9      |
| **ADC activo**         | ADC1                                      | Conversión continua de señal EMG proveniente del AD8232.                             |    14.1      |
| **I2C activo**         | I2C1 (OLED + EEPROM)                      | Comunicación con OLED y acceso a memoria I2C, refresco de pantalla.                  |    13.4      |
| **UART activo**        | USART1 (BLE HM-10)                        | Transmisión periódica de nivel de contracción al módulo Bluetooth HM-10.             |    11.4      |
| **Buzzer**             | GPIO                                      | Activación del buzzer cuando el nivel de contracción supera el umbral configurado.   |    13.2      |
| **Super Loop completo**| ADC + I2C + UART + GPIO                   | Funcionamiento total: adquisición EMG, procesamiento, transmisión BLE y OLED activo. |    14.5      |
<p align="center"><b>Tabla 1 — Escenarios de medición de consumo.</b></p>


| Tarea                   | Descripción                         | Tiempo de CPU (Ci) | Período (Ti) | Ci/Ti     |
|-------------------------|--------------------------------------|---------------------|--------------|-----------|
| **Adquisición EMG (ADC)**     | Muestreo continuo a 1 kHz              | 0.02 ms             | 1 ms         | 0.020     |
| **Procesamiento EMG**         | Filtrado, RMS y cálculo del nivel      | 0.40 ms             | 20 ms        | 0.020     |
| **Display OLED (I2C)**        | Refresco de pantalla                   | 1.50 ms             | 100 ms       | 0.015     |
| **Bluetooth BLE (UART)**      | Envío del valor procesado              | 0.30 ms             | 20 ms        | 0.015     |
| **Detección de umbral**       | Comparación y activación del buzzer    | 0.05 ms             | 20 ms        | 0.0025    |
| **Lectura de botón**          | Lectura periódica de entrada digital   | 0.02 ms             | 50 ms        | 0.0004    |

<p align="center"><b>Tabla — Tareas periódicas consideradas para el factor de carga.</b></p>

**Factor de uso total del sistema:**  
u = 0.020 + 0.020 + 0.015 + 0.015 + 0.0025 + 0.0004 = **0.0729 → 7.29%**




