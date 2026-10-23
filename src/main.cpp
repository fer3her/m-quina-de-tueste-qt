
// - Lee una fila del CSV cada 500 ms.
// - Dibuja las curvas cada 1 segundo.
// - Alarma visual si la temperatura del grano supera 215 C.
// - Boton para reiniciar la graficacion.

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPainter>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


static const double kAlarmTemp = 215.0;               // limite de la alarma (grados)
static const char*  kDefaultCsv = "data/roaster_generated.csv";   // CSV por defecto (cambia el nombre)


// Conversion: milivoltios -> grados (ecuacion del enunciado)

static double mvToTemp(double mv) {
    return 20.625 * ((mv / 150.0) - 4.0) + 20.0;
}


// Lectura del CSV

static std::string trim(std::string s) {
    while (!s.empty() && (s.back() == '\r' || s.back() == ' ')) s.pop_back();
    size_t i = 0;
    while (i < s.size() && s[i] == ' ') ++i;
    return s.substr(i);
}

// Parte una linea en celdas usando el separador (, o ;).
static std::vector<std::string> split(const std::string& line, char sep) {
    std::vector<std::string> out;
    std::stringstream ss(line);
    std::string item;
    while (std::getline(ss, item, sep)) out.push_back(item);
    return out;
}

// El CSV ya leido: los nombres de las columnas y una fila de numeros por linea.
struct CsvData {
    std::vector<std::string> header;
    std::vector<std::vector<double>> rows;
};

// Una lectura ya convertida a temperatura.
struct Sample {
    double t;      // segundos desde el inicio
    double bean;   // temperatura del grano
    double air;    // temperatura del aire
};

// Lee TODO el archivo una sola vez y lo guarda en memoria.
static CsvData loadCsv(const std::string& path) {
    CsvData data;
    std::ifstream file(path);
    std::string line;
    if (!std::getline(file, line)) return data;   // archivo vacio o inexistente

    // Detecta si el separador es ; o ,
    char sep = std::count(line.begin(), line.end(), ';') >
               std::count(line.begin(), line.end(), ',') ? ';' : ',';

    // La primera linea son los nombres de las columnas.
    for (auto& h : split(line, sep)) data.header.push_back(trim(h));

    // Las demas lineas son los datos.
    while (std::getline(file, line)) {
        auto cells = split(line, sep);
        if (cells.size() < data.header.size()) continue;   // linea incompleta: se ignora

        std::vector<double> row;
        for (auto& c : cells) {
            if (sep == ';') std::replace(c.begin(), c.end(), ',', '.');   // decimal con coma
            try { row.push_back(std::stod(c)); }
            catch (...) { row.push_back(NAN); }                           // celda no numerica
        }
        data.rows.push_back(row);
    }
    return data;
}

// ---------------------------------------------------------------
// Programa principal

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // 1) Leer el CSV (la ruta puede pasarse como argumento)
    std::string path = (argc > 1) ? argv[1] : kDefaultCsv;
    CsvData data = loadCsv(path);
    if (data.rows.empty()) {
        std::cerr << "No se pudo leer el CSV: " << path << '\n';
        return 1;
    }

    // Busca en que posicion esta cada columna por su nombre.
    auto indexOf = [&](const std::string& name) {
        for (size_t i = 0; i < data.header.size(); ++i)
            if (data.header[i] == name) return (int)i;
        return -1;
    };
    int beanCol = indexOf("bean_mv");
    int airCol  = indexOf("air_mv");
    if (beanCol < 0 || airCol < 0) {
        std::cerr << "Faltan las columnas bean_mv / air_mv\n";
        return 1;
    }

    // 2) Grafica: dos curvas (series) dentro de un grafico con dos ejes
    auto* beanSeries = new QLineSeries; beanSeries->setName("bean_temperature");
    auto* airSeries  = new QLineSeries; airSeries->setName("aire_temperature");

    auto* chart = new QChart;
    chart->addSeries(beanSeries);
    chart->addSeries(airSeries);
    chart->setTitle("Curvas de tueste");

    auto* axisX = new QValueAxis; axisX->setTitleText("Tiempo (s)");      axisX->setRange(0, 60);
    auto* axisY = new QValueAxis; axisY->setTitleText("Temperatura (C)"); axisY->setRange(0, 250);
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);
    for (auto* s : {beanSeries, airSeries}) {
        s->attachAxis(axisX);
        s->attachAxis(axisY);
    }

    auto* chartView = new QChartView(chart);       // el widget que muestra el grafico
    chartView->setRenderHint(QPainter::Antialiasing);

    // 3) Alarma visual y boton de reinicio
       
    auto* alarmLabel = new QLabel("Temperatura normal");
    alarmLabel->setAlignment(Qt::AlignCenter);
    alarmLabel->setMinimumHeight(40);
    alarmLabel->setStyleSheet("background:#2e7d32;color:white;font-weight:bold;font-size:16px;");

    // Parpadeo rojo/naranja
    auto* blinkTimer = new QTimer(alarmLabel);
    bool rojo = true;
    QObject::connect(blinkTimer, &QTimer::timeout, [=]() mutable {
        alarmLabel->setStyleSheet(QString("background:%1;color:white;font-weight:bold;font-size:16px;")
                                  .arg(rojo ? "#d32f2f" : "#ff9800"));
        rojo = !rojo;
    });

    auto* resetButton = new QPushButton("Reiniciar graficacion");

    // 4) Ventana: fila de arriba (alarma + boton) y debajo la grafica
    auto* top = new QHBoxLayout;
    top->addWidget(alarmLabel, 1);
    top->addWidget(resetButton);

    auto* central = new QWidget;
    auto* layout = new QVBoxLayout(central);
    layout->addLayout(top);
    layout->addWidget(chartView, 1);

    QMainWindow window;
    window.setWindowTitle("Monitor de tueste");
    window.setCentralWidget(central);
    window.resize(1000, 600);
    window.show();

    // 5) Estado del programa
    std::vector<Sample> pending;   // lecturas ya hechas pero aun no dibujadas
    size_t nextRow = 0;            // siguiente fila del CSV por leer
    bool alarmOn = false;          // si la alarma esta encendida ahora

    // Enciende o apaga la alarma visual.
      auto setAlarm = [&](bool on) {
        if (on == alarmOn) return;
        alarmOn = on;
        if (on) {
            alarmLabel->setText(QString("ALARMA: temperatura del grano > %1 C").arg(kAlarmTemp));
            blinkTimer->start(500);      // empieza a parpadear
        } else {
            blinkTimer->stop();          // deja de parpadear
            alarmLabel->setText("Temperatura normal");
            alarmLabel->setStyleSheet("background:#2e7d32;color:white;font-weight:bold;font-size:16px;");
        }
    };

    // 6) Temporizador de LECTURA: cada 500 ms toma una fila, la convierte y revisa la alarma
    QTimer readTimer;
    readTimer.setInterval(500);
    QObject::connect(&readTimer, &QTimer::timeout, [&] {
        if (nextRow >= data.rows.size()) {   // se acabo el archivo
            readTimer.stop();
            return;
        }
        const auto& row = data.rows[nextRow];
        Sample s{ nextRow * 0.5, mvToTemp(row[beanCol]), mvToTemp(row[airCol]) };
        ++nextRow;

        if (std::isnan(s.bean) || std::isnan(s.air)) return;   // dato invalido: se salta

        pending.push_back(s);
        setAlarm(s.bean > kAlarmTemp);       // alarma si el grano supera 215 C
    });

    // 7) Temporizador de DIBUJO: cada 1000 ms pinta todo lo que se haya acumulado
    QTimer plotTimer;
    plotTimer.setInterval(1000);
    QObject::connect(&plotTimer, &QTimer::timeout, [&] {
        for (const auto& s : pending) {
            beanSeries->append(s.t, s.bean);
            airSeries->append(s.t, s.air);
        }
        if (!pending.empty())
            axisX->setRange(0, std::max(60.0, pending.back().t + 5));   // el eje X crece
        pending.clear();
    });

    // 8) Boton de reinicio: borra las curvas y vuelve a empezar desde la primera fila
    QObject::connect(resetButton, &QPushButton::clicked, [&] {
        beanSeries->clear();
        airSeries->clear();
        pending.clear();
        nextRow = 0;
        setAlarm(false);
        axisX->setRange(0, 60);
        readTimer.start();   // por si el archivo ya habia terminado
    });

    readTimer.start();
    plotTimer.start();
    return app.exec();       // mantiene la aplicacion funcionando
}