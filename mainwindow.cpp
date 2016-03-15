#include "mainwindow.h"
#include <cmath>
#include <iostream>
#include <QSerialPortInfo>
#include <QSettings>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    this->setWindowTitle("Mock Monitor");

    // Parameter initialization
    n_curves = 4;
    sampling_interval_ms = 10;
    n_plot_points = 1000;
    plotting_cnt = 0;

    // Plotting points initialization.
    plot_x = new double[n_plot_points];
    plot_y = new double*[n_curves];
    for (int i=0; i<n_curves; i++) {
        this->plot_y[i] = new double[n_plot_points]();
    }
    for (int i=0; i<n_plot_points; i++) {
        plot_x[i] = (float) i * sampling_interval_ms;
    }

    // UI initialization.
    ui->setupUi(this);

    // Curves initialization.
    QString color_names[] = {"red", "green", "blue", "orange", "purple"};
    curves = new QwtPlotCurve*[n_curves];
    for (int i=0; i<n_curves; i++) {
        QColor c(color_names[i]);
        c.setAlphaF(0.8);
        curves[i] = new QwtPlotCurve(QString("curve%1").arg(i));
        curves[i]->setPen(c, 1.5);
        curves[i]->attach( ui->qwtPlot );
    }

    // Graph initialization
    ui->qwtPlot->setAxisTitle(QwtPlot::xBottom, "time [ms]");
    ui->qwtPlot->setAxisTitle(QwtPlot::yLeft, "Pressure [mmHg]");
    ui->qwtPlot->setAxisScale(0, -100, 200); // Y-axis
    ui->qwtPlot->setAxisScale(1, 0, sampling_interval_ms * n_plot_points); // X-axis
    ui->qwtPlot->setCanvasBackground(QColor(Qt::lightGray));

    // Grid initialization.
    grid1 = new QwtPlotGrid;
    grid1->setPen(Qt::gray);
    // grid1->attach(ui->qwtPlot);

    // Marker init
    marker1 = new QwtPlotMarker;
    marker1->setLineStyle(QwtPlotMarker::VLine);
    marker1->setLinePen(QPen(Qt::yellow));
    marker1->attach(ui->qwtPlot);

    // Replotting timer.
    timer1 = new QTimer();
    timer1->start(50);
    connect(timer1, SIGNAL(timeout()), this, SLOT(redraw()));

    // Serial port list.
    QList<QSerialPortInfo> portlist = QSerialPortInfo::availablePorts();
    for (int i=portlist.size()-1; i>=0; i--) {
        QString portname = portlist.at(i).portName();
        portname += ":";
        portname += portlist.at(i).description();
        ui->comboBox_CommPort->addItem(portname);
    }

    // Serial baudrate list.
    QList<qint32> baudlist = QSerialPortInfo::standardBaudRates();
    for (int i=0; i<baudlist.size(); i++) {
        qint32 baud_int = baudlist.at(i);
        QString baud_str = QString("%1").arg(baud_int);
        ui->comboBox_CommBaudrate->addItem(baud_str);
    }

    // Serial port settings.
    serial1 = new QSerialPort();
    connect(serial1, SIGNAL(readyRead()), this, SLOT(readData()));

    // Load setting data.
    cfg = new QSettings("config.ini", QSettings::IniFormat);
    cfg->setIniCodec("UTF-8");

    // Load the serial port setting.
    QString portname = cfg->value("Serial/Port", "COM1").toString();
    int port_idx = ui->comboBox_CommPort->findText(portname);
    if ( port_idx != -1 ) {
        ui->comboBox_CommPort->setCurrentIndex( port_idx );
    }

    // Load the current settings.
    QString baudrate = cfg->value("Serial/Baudrate", "9800").toString();
    int baud_idx = ui->comboBox_CommBaudrate->findText(baudrate);
    if ( baud_idx != -1 ) {
        ui->comboBox_CommBaudrate->setCurrentIndex(baud_idx);
    }
}

// Destractor.
MainWindow::~MainWindow()
{
    // Save the current settings.
    cfg->setValue("Serial/Port", ui->comboBox_CommPort->currentText());
    cfg->setValue("Serial/Baudrate", ui->comboBox_CommBaudrate->currentText());
    delete cfg;

    // Delete all allocated resources.
    delete ui;
    for (int i=0; i<n_curves; i++)
        delete curves[i];
    delete curves;
    delete timer1;

    delete serial1;

    delete plot_x;
    for (int i=0; i<n_curves; i++) {
        delete plot_y[i];
    }
    delete plot_y;
}

// Replot the graph.
void MainWindow::redraw()
{
    for (int i=0; i<this->n_curves; i++) {
        curves[i]->setSamples(this->plot_x, this->plot_y[i], n_plot_points);
    }
    marker1->setXValue(plotting_cnt * sampling_interval_ms);
    ui->qwtPlot->replot();
}

// Handle received data.
void MainWindow::readData()
{
    serial1_queue += serial1->readAll();

    int j;
    while ( (j = serial1_queue.indexOf("\r\n")) != -1 ) {
        // Extraction of the current processing line and eliminate it for next processing.
        QByteArray line = serial1_queue.mid(0, j);
        serial1_queue.remove(0, j+2);
        // A process of current line.
        QList<QByteArray> elems = line.split(',');
        int n_elems = elems.length();
        if ( n_elems != n_curves ) {
            //  this->ui->textBrowser->setText(line);
            continue;
        }
        for (int i=0; i<n_curves; i++) {
            double value = (double) elems[i].toDouble();
            plot_y[i][plotting_cnt] = value;
        }
        plotting_cnt = (plotting_cnt + 1) % n_plot_points;
    }
}

// Handle serial connection mode.
void MainWindow::on_pushButton_ConnectComm_clicked()
{
    if ( serial1->isOpen() ) {
        serial1->close();
        ui->pushButton_ConnectComm->setText("Connect");
        ui->statusbar->showMessage("The port is closed", 1000);
    } else {
        // Set port name from the combo box.
        QString portname = ui->comboBox_CommPort->currentText();
        int port_sep_idx = portname.indexOf(":");
        portname = portname.left(port_sep_idx);
        serial1->setPortName(portname);
        // Set baudrate from the combo box.
        qint32 baudrate = ui->comboBox_CommBaudrate->currentText().toInt();
        serial1->setBaudRate(baudrate);
        // Reset the starting line.
        plotting_cnt = 0;

        if ( serial1->open(QIODevice::ReadWrite) == false ) { // Failed to open the port.
            ui->statusbar->showMessage("Failed to open COM port", 1000);
        } else {
            ui->pushButton_ConnectComm->setText("Disconnect");
            ui->statusbar->showMessage("The port is opened.", 1000);
        }
    }
}

// Clear the plotting data.
void MainWindow::on_pushButton_Clear_clicked()
{
    for (int i=0; i<n_plot_points; i++) {
        for (int j=0; j<n_curves; j++) {
            this->plot_y[j][i] = 0.0;
        }
    }
    plotting_cnt = 0;
}

void MainWindow::on_pushButton_Grid_clicked()
{
    if ( grid1->plot() == 0 ) { // Not attached yet.
        grid1->attach(ui->qwtPlot);
        ui->pushButton_Grid->setText("Grid Off");
    } else {
        grid1->detach();
        ui->pushButton_Grid->setText("Grid On");
    }
}

void MainWindow::on_pushButton_device_transmit_clicked()
{
    if ( serial1->isOpen() == false ) {
        ui->statusbar->showMessage("Port isn't opened.", 1000);
        return;
    }

    QString mode = ui->comboBox_device_mode->currentText();
    if ( mode == "Reset" )  serial1->sendBreak();
    else if ( mode == "Command") {
        QString cmd = ui->plainTextEdit_device_command->toPlainText();
        serial1->write( cmd.toStdString().c_str() );
    } else {
        ui->statusbar->showMessage("Unknown command was selected.", 1000);
    }
}
