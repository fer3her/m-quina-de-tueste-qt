
# Monitoreo de maquina de tueste (Qt)

Aplicación de escritorio que simula el monitoreo de un tostador de café. Lee temperaturas desde un archivo CSV, las convierte a grados y dibuja las curvas en tiempo real. Si la temperatura del grano supera el límite, activa una alarma visual que parpadea en rojo y naranja.

Características
Lee una fila del CSV cada 500 ms.
Dibuja las curvas de temperatura del grano y del aire cada 1 segundo.
Alarma visual  cuando la temperatura del grano supera el límite.
Botón Reiniciar graficación: borra las curvas y vuelve a empezar desde la primera fila.

## Cada lectura del CSV viene en milivoltios y se convierte a grados con

T = 20.625 * ((mv / 150) - 4) + 20

## Formato del CSV El archivo debe tener una fila de encabezado con al menos estas columnas

Columna Descripción
bean_mv Señal del sensor del grano (mV)
air_mv Señal del sensor del aire (mV)

Por defecto el programa busca data/roaster.csv. También puedes indicar otra ruta como argumento:

## Compilar y ejecutar

cmake -S . -B build
cmake --build build
.\build\roastmonitor.exe

## Configuración

El límite de la alarma se cambia en src/main.cpp:

cpp
static const double kAlarmTemp = 215.0;   // grados

Estructura del proyecto
.
├── CMakeLists.txt
├── data/
│   └── roasted_generated data.csv
└── src/
    └── main.cpp.
