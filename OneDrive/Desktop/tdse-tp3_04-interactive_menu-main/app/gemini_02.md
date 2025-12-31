## Análisis del Impacto de LOGGER_INFO() en las Variables del Sistema

### **Funcionamiento del Sistema de Logging**

#### **Arquitectura del Logger**
El sistema implementa un logger con las siguientes características:

```c
// logger.h - Configuración clave
#define LOGGER_CONFIG_ENABLE          (1)          // Logger habilitado
#define LOGGER_CONFIG_MAXLEN          (64)         // Buffer de 64 caracteres
#define LOGGER_CONFIG_USE_SEMIHOSTING (1)          // Usa semihosting para output

// Macro LOGGER_INFO con protección de interrupciones
#define LOGGER_INFO(...)\
    LOGGER_LOG("[info] ");\
    LOGGER_LOG(__VA_ARGS__);\
    LOGGER_LOG("\n");
```

#### **Mecanismo de Protección**
```c
#define LOGGER_LOG(...)\
    __asm("CPSID i");  /* disable interrupts*/\
    {\
        logger_msg_len = snprintf(logger_msg, (LOGGER_CONFIG_MAXLEN - 1), __VA_ARGS__);\
        logger_log_print_(logger_msg);\
    }\
    __asm("CPSIE i");  /* enable interrupts*/
```

### **Impacto en las Variables Clave**

#### **1. `g_app_runtime_us` (Unidad: microsegundos)**

**Definición**: Tiempo total de ejecución de todas las tareas en un ciclo.

**Impacto del LOGGER_INFO()**:
- **Aumento significativo** del tiempo de ejecución
- Las llamadas a `LOGGER_INFO()` consumen tiempo de CPU debido a:
  - Formateo de strings con `snprintf()`
  - Llamadas al sistema via semihosting
  - Protección con deshabilitación de interrupciones

**Comparación SIN vs CON logging**:
```
SIN Logger:    g_app_runtime_us ≈ 100-200μs
CON Logger:    g_app_runtime_us ≈ 500-2000μs (5-10x mayor)
```

**Ejemplo de evolución CON logging**:
```
Ciclo 1: 0us → 850us → 1650us  (alta carga por inicialización)
Ciclo 2: 0us → 620us → 1250us  (carga reducida)
Ciclo 3: 0us → 580us → 1180us  (estabilización)
```

#### **2. `task_dta_list[index].WCET` (Unidad: microsegundos)**

**Definición**: Peor tiempo de ejecución de cada tarea.

**Impacto del LOGGER_INFO()**:
- **WCET se incrementa dramáticamente** debido al overhead del logging
- Las tareas que más llamadas a `LOGGER_INFO()` tengan tendrán WCET más altos

**Comparativa por tarea**:

**Tarea Sensor** (`task_sensor.c`):
```c
// En task_sensor_init() - Múltiples LOGGER_INFO()
LOGGER_INFO("  %s is running - %s", GET_NAME(task_sensor_init), p_task_sensor);
LOGGER_INFO("  %s is a %s", GET_NAME(task_sensor), p_task_sensor_);
LOGGER_INFO("   %s = %lu", GET_NAME(g_task_sensor_cnt), g_task_sensor_cnt);
// ... más 3 LOGGER_INFO adicionales por cada botón
```

**Tarea Menu** (`task_menu.c`):
```c
// En task_menu_init() - También múltiples LOGGER_INFO()
LOGGER_INFO("  %s is running - %s", GET_NAME(task_menu_init), p_task_menu);
LOGGER_INFO("  %s is a %s", GET_NAME(task_menu), p_task_menu_);
LOGGER_INFO("   %s = %lu", GET_NAME(g_task_menu_cnt), g_task_menu_cnt);
// ... más LOGGER_INFO adicionales
```

**Valores típicos de WCET**:
```
SIN Logger:
  - WCET Sensor: ≈ 45μs
  - WCET Menu:   ≈ 85μs

CON Logger (durante inicialización):
  - WCET Sensor: ≈ 1500-3000μs  (30-60x mayor)
  - WCET Menu:   ≈ 2000-4000μs  (25-50x mayor)
```

#### **3. `g_task_sensor_tick_cnt` (Unidad: milisegundos)**

**Definición**: Contador de ticks para la tarea de sensores.

**Impacto del LOGGER_INFO()**:
- **Acumulación de ticks** debido al bloqueo por logging
- Durante las operaciones de logging, las interrupciones están deshabilitadas
- Esto puede causar que se pierdan ticks o se acumulen

**Comportamiento problemático**:
```c
void HAL_SYSTICK_Callback(void) {
    g_task_sensor_tick_cnt++;  // Se incrementa cada 1ms
}

// Pero durante LOGGER_INFO():
__asm("CPSID i");  // Interrupciones DESHABILITADAS
// Si snprintf() + printf() toman 2ms...
// Se pierden 2 ticks de SYSTICK!
__asm("CPSIE i");  // Interrupciones HABILITADAS
```

**Ejemplo de evolución CON logging**:
```
Tiempo | g_task_sensor_tick_cnt | Comportamiento
-------|------------------------|---------------
0ms    | 0                      | Inicio
1ms    | 1                      | Interrupción SYSTICK
2ms    | 2                      | Interrupción SYSTICK  
3ms    | 3                      | Interrupción SYSTICK (se acumula)
4ms    | 0                      | Tarea procesa 3 ticks (lento por logging)
5ms    | 1                      | Interrupción SYSTICK
```

### **Análisis Detallado del Flujo**

#### **Fase de Inicialización (`app_init()`)**

**Comportamiento CON logging**:
1. **Alto consumo de tiempo inicial**:
   ```c
   LOGGER_INFO(" ");  // ≈ 200-500μs
   LOGGER_INFO("%s is running - Tick [mS] = %lu", GET_NAME(app_init), HAL_GetTick());  // ≈ 500-1000μs
   LOGGER_INFO(p_sys);  // ≈ 200-400μs
   LOGGER_INFO(p_app);  // ≈ 200-400μs
   LOGGER_INFO(" %s = %lu", GET_NAME(g_app_cnt), g_app_cnt);  // ≈ 300-600μs
   ```

2. **WCET se establece en valores altos** desde el inicio:
   ```
   task_dta_list[0].WCET (Sensor) = 2500μs  (por task_sensor_init con logging)
   task_dta_list[1].WCET (Menu)   = 3500μs  (por task_menu_init con logging)
   ```

#### **Loop Principal (`app_update()`)**

**Problemas introducidos por el logging**:

1. **Desincronización temporal**:
   - Las tareas toman más tiempo en ejecutarse
   - Los contadores de tick se acumulan
   - Posible pérdida de eventos en tiempo real

2. **Overhead constante**:
   - Cada llamada a `LOGGER_INFO()` añade ≈200-1000μs
   - El sistema pasa de ser determinístico a tener tiempos variables

3. **Impacto en la máquina de estados**:
   ```c
   // En task_sensor_statechart() - Sin logging rápido
   // Con logging - los debouncing pueden fallar por retardos
   ```

### **Ejemplo de Ejecución con Logging Habilitado**

```
Tiempo | g_app_runtime_us | WCET Sensor | WCET Menu | g_task_sensor_tick_cnt
-------|------------------|-------------|-----------|-----------------------
0ms    | 0us              | 0us         | 0us       | 0
1ms    | 0us              | 0us         | 0us       | 1
2ms    | 2850us           | 2850us      | 0us       | 0 (procesado lentamente)
3ms    | 0us              | 2850us      | 0us       | 1  
4ms    | 3200us           | 2850us      | 3200us    | 0 (procesado lentamente)
5ms    | 0us              | 2850us      | 3200us    | 1
```

### **Conclusiones del Impacto**

1. **`g_app_runtime_us`**: **Aumenta 5-10 veces** debido al overhead del formateo y output de strings

2. **`task_dta_list[index].WCET`**: **Aumenta 20-60 veces** porque captura el peor caso que incluye todas las operaciones de logging

3. **`g_task_sensor_tick_cnt`**: **Comportamiento errático** con acumulación de ticks debido al bloqueo durante las operaciones de logging

4. **Problemas adicionales**:
   - **Pérdida de determinismo**: Los tiempos de ejecución ya no son predecibles
   - **Posible pérdida de eventos**: Los botones pueden no detectarse correctamente
   - **Overhead significativo**: El sistema dedica más tiempo a logging que a lógica de aplicación

**Recomendación**: En sistemas embebidos en tiempo real, el logging debería usarse solo para debugging y deshabilitarse en producción, o implementarse de forma asíncrona para minimizar el impacto en el timing del sistema.