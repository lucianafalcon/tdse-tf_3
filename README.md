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

| Elemento           | Definición |
|-------------------|------------|
| **Disparador**     | El usuario contrae el músculo mientras el sistema está adquiriendo la señal. |
| **Precondiciones** | El sistema está encendido. Los electrodos están correctamente colocados. El módulo EMG V3 entrega señal al microcontrolador. El display OLED está operativo. |
| **Flujo principal** | El ADC del STM32 toma muestras periódicas de la señal EMG. El sistema filtra y procesa la señal digitalmente. Se calcula el nivel de activación muscular. El valor procesado se muestra en tiempo real en el display OLED. Si el nivel supera el umbral configurado, se activa el LED o buzzer. |
| **Flujos alternativos** | a. Ruido excesivo: se muestra advertencia o se ignora la medición. <br> b. Señal fuera de rango: el sistema solicita recalibración. <br> c. Interrupción manual: el usuario presiona el botón para detener la adquisición. |

<p align="center"><b>Caso de Uso 1 – Detección de Contracción Muscular</b></p>


| Elemento           | Definición |
|-------------------|------------|
| **Disparador**     | Un dispositivo móvil o PC se conecta al módulo Bluetooth del sistema. |
| **Precondiciones** | Bluetooth HM-10 encendido y emparejado. Sistema en modo transmisión. |
| **Flujo principal** | El sistema inicia el envío continuo de datos EMG procesados. La PC o dispositivo móvil recibe y grafica los datos. La transmisión continúa mientras haya conexión. |
| **Flujos alternativos** | a. Pérdida de conexión: el sistema pausa la transmisión y espera reconexión. <br> b. Exceso de datos: el sistema reduce la frecuencia de muestreo temporalmente. |

<p align="center"><b>Caso de Uso 2 – Envío de Datos por Bluetooth</b></p>


| Elemento           | Definición |
|-------------------|------------|
| **Disparador**     | El usuario inicia el modo de calibración desde un botón o comando. |
| **Precondiciones** | Sistema encendido. Electrodos colocados correctamente. |
| **Flujo principal** | El usuario mantiene el músculo relajado durante unos segundos. El sistema mide el nivel base de ruido y deriva un valor de referencia. Se calcula automáticamente un umbral óptimo de detección. El sistema indica que la calibración fue exitosa. |
| **Flujos alternativos** | a. Movimiento durante calibración: el sistema solicita repetir el proceso. <br> b. Señal inválida: se muestra error y no se actualiza el umbral. |

<p align="center"><b>Caso de Uso 3 – Calibración Automática del Umbral</b></p>


