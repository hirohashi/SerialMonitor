#ifndef _MAINWINDOW_
#define _MAINWINDOW_

#include "ui_mockmonitor.h"

#include <qwt_plot_curve.h>
#include <qwt_plot_grid.h>
#include <qwt_plot_marker.h>
#include <qwt_plot_legenditem.h>

#include <QMainWindow>
#include <QTimer>
#include <QtSerialPort>
#include <QSettings>

class MainWindow : public QMainWindow
{
Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void readData();
    void redraw();
    void on_pushButton_Clear_clicked();
    void on_pushButton_ConnectComm_clicked();

    void on_pushButton_Grid_clicked();
    void on_pushButton_device_transmit_clicked();

private:
    int n_curves;
    QwtPlotCurve **curves;
    QwtPlotGrid *grid1;
    QwtPlotMarker *marker1;
    int n_plot_points;
    int sampling_interval_ms;
    int plotting_cnt;
    double *plot_x;
    double **plot_y;

    Ui::MainWindow *ui;
    QTimer *timer1;
    QSerialPort *serial1;
    QByteArray serial1_queue;

    QSettings *cfg;
};

#endif

