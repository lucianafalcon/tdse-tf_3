## Análisis del Código Fuente

Voy a analizar el funcionamiento del sistema embebido basado en STM32F1xx y la evolución de las variables solicitadas.

### **Arquitectura General del Sistema**

El sistema implementa un **sistema de tiempo real no preemptivo (ETS - Event-Triggered Systems)** con las siguientes características:

- **Tareas cooperativas**: `task_sensor` y `task_menu`
- **Gestión de eventos**: Cola de eventos para comunicación entre tareas
- **Control de display LCD**: 20x4 caracteres
- **Manejo de botones**: ENT, NEX, ESC con debouncing

### **Evolución de las Variables Clave**

#### **1. `g_app_runtime_us` (Unidad: microsegundos)**

**Definición**: Tiempo total de ejecución de todas las tareas en un ciclo de la aplicación.

**Comportamiento**:
- **Inicialización**: `0` en `app_init()`
- **En `app_update()`**:
  - Se **reinicia a 0** al inicio de cada procesamiento de tick
  - Se **incrementa** sumando el tiempo de ejecución de cada tarea medida con `cycle_counter_get_time_us()`
  - Representa el **tiempo total** que tarda en ejecutarse todas las tareas en un ciclo

**Ejemplo de evolución**:
```
Ciclo 1: 0us → 150us → 280us (acumulando tareas)
Ciclo 2: 0us → 140us → 270us
Ciclo 3: 0us → 160us → 290us
```

#### **2. `task_dta_list[index].WCET` (Unidad: microsegundos)**

**Definición**: **Worst-Case Execution Time** - Peor tiempo de ejecución de cada tarea.

**Comportamiento**:
- **Inicialización**: `0` en `app_init()`
- **En `app_update()`**:
  - Se compara el tiempo actual de ejecución (`cycle_counter_time_us`) con el WCET almacenado
  - Si el tiempo actual **es mayor**, se **actualiza** el WCET
  - Solo **crece monotónicamente** - nunca disminuye

**Ejemplo de evolución**:
```
Tarea Sensor:   0us → 45us → 45us → 48us → 48us (nuevo máximo)
Tarea Menu:     0us → 85us → 85us → 85us → 90us (nuevo máximo)
```

#### **3. `g_task_sensor_tick_cnt` (Unidad: milisegundos)**

**Definición**: Contador de ticks para la tarea de sensores.

**Comportamiento**:
- **Inicialización**: `0` en `app_init()`
- **Incremento**: Se incrementa en **1 ms** en `HAL_SYSTICK_Callback()`
- **Decremento**: Se decrementa en `task_sensor_update()` cuando se procesan ticks pendientes

**Mecanismo de control**:
```c
// En la interrupción (cada 1ms)
void HAL_SYSTICK_Callback(void) {
    g_task_sensor_tick_cnt++;  // Incrementa
}

// En la tarea
void task_sensor_update(void *parameters) {
    if (g_task_sensor_tick_cnt > 0) {
        g_task_sensor_tick_cnt--;  // Decrementa al procesar
        // Ejecutar lógica de la tarea
    }
}
```

**Ejemplo de evolución**:
```
Tiempo (ms) | Valor | Comportamiento
------------|-------|---------------
0           | 0     | Inicializado
1           | 1     | Interrupción SYSTICK
2           | 2     | Interrupción SYSTICK  
3           | 1     | Tarea procesa 1 tick
4           | 2     | Interrupción SYSTICK
5           | 0     | Tarea procesa 2 ticks
```

### **Flujo de Ejecución Detallado**

#### **Fase de Inicialización (`app_init()`)**

1. **Inicialización de variables**:
   ```c
   g_app_cnt = 0;
   g_app_tick_cnt = 0;
   g_task_sensor_tick_cnt = 0;
   g_task_menu_tick_cnt = 0;
   task_dta_list[index].WCET = 0;  // Para cada tarea
   ```

2. **Inicialización de tareas**:
   - `task_sensor_init()`: Configura FSM para botones
   - `task_menu_init()`: Inicializa display LCD y cola de eventos

3. **Display LCD**: Muestra "TdSE Bienvenidos" y "Test Nro: "

#### **Loop Principal (`app_update()`)**

1. **Verificación de ticks pendientes**:
   ```c
   if (g_app_tick_cnt > 0) {
       g_app_tick_cnt--;
       b_time_update_required = true;
   }
   ```

2. **Procesamiento de tareas**:
   ```c
   while (b_time_update_required) {
       g_app_runtime_us = 0;  // Reset contador de tiempo
       
       for (cada tarea) {
           cycle_counter_reset();
           ejecutar_tarea();
           cycle_counter_time_us = cycle_counter_get_time_us();
           
           g_app_runtime_us += cycle_counter_time_us;  // Acumular tiempo
           
           // Actualizar WCET si es necesario
           if (task_dta_list[index].WCET < cycle_counter_time_us) {
               task_dta_list[index].WCET = cycle_counter_time_us;
           }
       }
   }
   ```

### **Comportamiento de las Tareas**

#### **`task_sensor` - Máquina de Estados de Botones**
- Detecta pulsaciones de botones (ENT, NEX, ESC)
- Implementa debouncing mecánico
- Genera eventos para el menú (`put_event_task_menu()`)

#### **`task_menu` - Menú Interactivo**
- Procesa eventos de botones desde la cola
- Actualiza display LCD cada 500ms cuando está activo
- Muestra contador de ejecuciones en formato: `(g_task_menu_cnt/1000)`

### **Patrón de Evolución Típico**

```
Tiempo | g_app_runtime_us | WCET Sensor | WCET Menu | g_task_sensor_tick_cnt
-------|------------------|-------------|-----------|-----------------------
0ms    | 0us              | 0us         | 0us       | 0
1ms    | 0us              | 0us         | 0us       | 1 ↑
2ms    | 130us            | 45us        | 85us      | 0 ↓ (procesado)
3ms    | 0us              | 45us        | 85us      | 1 ↑
4ms    | 125us            | 45us        | 85us      | 0 ↓ (procesado)
5ms    | 0us              | 45us        | 85us      | 1 ↑
```

### **Conclusiones**

1. **`g_app_runtime_us`**: Varía según la carga computacional de cada ciclo, típicamente entre 100-300μs
2. **`task_dta_list[index].WCET`**: Crece hasta estabilizarse en el peor caso de ejecución encontrado
3. **`g_task_sensor_tick_cnt`**: Oscila entre 0-2, indicando ticks pendientes de procesar

El sistema implementa un **schedule cooperativo** donde las tareas se ejecutan secuencialmente cuando hay ticks disponibles, con medición precisa de tiempos de ejecución para análisis de performance.