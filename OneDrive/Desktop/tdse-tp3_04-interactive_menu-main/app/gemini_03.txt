## Análisis de los Módulos de Hardware y Timing

### **1. board.h - Configuración Específica de Placa**

#### **Propósito**
Este archivo proporciona una **capa de abstracción de hardware** que permite que el mismo código fuente funcione en diferentes placas STM32 sin modificaciones.

#### **Arquitectura de Definiciones**

```c
// Identificación de placas soportadas
#define NUCLEO_F103RC    (0)
#define NUCLEO_F303R8    (1)
// ... más definiciones
#define BOARD (NUCLEO_F103RC)  // Placa actual seleccionada
```

#### **Configuración por Familia de Placas**

**Para Nucleo de 64 pines (F103RC, F401RE, F446RE):**
```c
#define BTN_ENT_PIN      D10_Pin
#define BTN_ENT_PORT     D10_GPIO_Port
#define BTN_ENT_PRESSED  GPIO_PIN_RESET    // Botón presionado = 0 (pull-up)
#define BTN_ENT_HOVER    GPIO_PIN_SET      // Botón liberado = 1

#define LED_A_PIN        LD2_Pin
#define LED_A_PORT       LD2_GPIO_Port  
#define LED_A_ON         GPIO_PIN_SET      // LED encendido = 1
#define LED_A_OFF        GPIO_PIN_RESET    // LED apagado = 0
```

**Características clave:**
- **Botones**: Configurados con resistencias pull-up (presionado = 0)
- **LEDs**: Configurados como salidas activas en alto (encendido = 1)
- **Portabilidad**: Mismo código funciona en F103RC, F401RE, F446RE

---

### **2. dwt.h - Contador de Ciclos de CPU para Medición de Tiempo**

#### **Propósito**
Implementa medición de tiempo de ejecución de código usando el **DWT (Data Watchpoint and Trace)** del Cortex-M, que es un contador de ciclos de CPU de 32 bits.

#### **Funciones Principales**

**Inicialización del contador:**
```c
static inline void cycle_counter_init(void)
{
     CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;  // Habilita DWT
     DWT->CYCCNT = 0;                                 // Reinicia contador
     DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;             // Inicia conteo
}
```

**Obtención de tiempo en microsegundos:**
```c
static inline uint32_t cycle_counter_get_time_us(void)
{
    return (DWT->CYCCNT / (SystemCoreClock / 1000000));
}
```

#### **Características Técnicas**
- **Precisión**: Un ciclo de CPU (máxima precisión posible)
- **Rango**: 32 bits → ~4.29 segundos a 100MHz
- **Overhead**: Cero (hardware dedicado)
- **Uso típico en app.c**:
  ```c
  cycle_counter_reset();
  ejecutar_tarea();
  cycle_counter_time_us = cycle_counter_get_time_us();  // Tiempo exacto en μs
  ```

---

### **3. systick.c - Delay de Precisión en Microsegundos**

#### **Propósito**
Implementa delays bloqueantes de alta precisión usando el timer SysTick del sistema.

#### **Algoritmo Implementado**

```c
void systick_delay_us(uint32_t delay_us)
{
    uint32_t start, current, target, elapsed;
    
    // Calcula número de ciclos necesarios
    target = delay_us * (SystemCoreClock / 1000000UL);
    
    start = SysTick->VAL;  // Valor inicial del contador
    
    while (1) {
        current = SysTick->VAL;
        
        // Maneja wrap-around del contador
        if (current <= start) {
            elapsed = start - current;
        } else {
            elapsed = SysTick->LOAD + start - current;
        }
        
        if (elapsed >= target) {
            break;
        }
    }
}
```

#### **Características del Delay**

**Precisión:**
- Basado en el reloj del sistema (`SystemCoreClock`)
- Ejemplo: Si `SystemCoreClock = 64MHz`:
  - `target = delay_us * 64` (ciclos por microsegundo)
  - Delay de 37μs → 37 × 64 = 2368 ciclos

**Manejo de Wrap-around:**
- SysTick es un contador descendente que se recarga
- El algoritmo detecta cuando el contador pasa por cero
- Calcula correctamente el tiempo transcurrido en ambos casos

**Uso en display.c:**
```c
// Delays críticos para el control del LCD
#define DISPLAY_DEL_37US    37ul
#define DISPLAY_DEL_01US    01ul

systick_delay_us(DISPLAY_DEL_37US);  // Delay preciso de 37μs
```

---

### **Relación entre los Módulos y su Impacto en las Variables**

#### **Para `g_app_runtime_us` (μs)**
- **Medido con DWT**: `cycle_counter_get_time_us()`
- **Precisión**: Sub-microsegundo (depende de la frecuencia de CPU)
- **Overhead**: Mínimo (solo lectura de registro hardware)

#### **Para `task_dta_list[index].WCET` (μs)**
- **Base de medición**: DWT cycle counter
- **Exactitud**: Reflecta el verdadero WCET de cada tarea
- **No incluye**: Tiempo de overhead del sistema de medición

#### **Para `g_task_sensor_tick_cnt` (ms)**
- **Relación con systick**: Incrementado por `HAL_SYSTICK_Callback()` cada 1ms
- **Independiente**: Los delays de systick.c no afectan este contador
- **Precisión**: Basada en el timer del sistema (SysTick)

---

### **Ejemplo de Integración en el Sistema**

```c
// En app_update() - Medición de tiempo de tareas
for (index = 0; TASK_QTY > index; index++) {
    cycle_counter_reset();                          // Reinicia DWT
    
    (*task_cfg_list[index].task_update)(...);      // Ejecuta tarea
    
    cycle_counter_time_us = cycle_counter_get_time_us();  // Lee DWT
    
    g_app_runtime_us += cycle_counter_time_us;     // Acumula tiempo total
    
    // Actualiza WCET si es necesario
    if (task_dta_list[index].WCET < cycle_counter_time_us) {
        task_dta_list[index].WCET = cycle_counter_time_us;
    }
}

// En display.c - Delays precisos para LCD
systick_delay_us(37);  // Timing exacto para control LCD
```

---

### **Ventajas de esta Arquitectura**

1. **Portabilidad**: `board.h` permite cambiar de placa con un solo #define
2. **Precisión temporal**: DWT proporciona medición de tiempo de ejecución exacta
3. **Delays confiables**: systick.c ofrece delays bloqueantes de microsegundo
4. **Bajo overhead**: Ambas soluciones usan hardware dedicado
5. **Determinismo**: Mediciones no afectadas por interrupciones o carga del sistema

Esta arquitectura es típica de sistemas embebidos en tiempo real donde el control preciso de timing es crítico para el funcionamiento correcto del sistema.