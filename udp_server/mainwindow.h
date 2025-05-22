//mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QTimer>
#include <QWidget>
#include <QString>
#include <Qt>
#include <QDebug>
#include "udp_server.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onStartClicked();
    void onStopClicked();
    void updateStatus();

private:
    QPushButton *startButton;
    QPushButton *stopButton;
    QLabel *statusLabel;

    UDPServer *server;

    QTimer *statusTimer;

    void setupUI();
};

#endif // MAINWINDOW_H

