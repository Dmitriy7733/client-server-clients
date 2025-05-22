//mainwindow.cpp

#include "mainwindow.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), server(nullptr)
{
    setupUI();
    statusTimer = new QTimer(this);
    connect(statusTimer, &QTimer::timeout, this, &MainWindow::updateStatus);
    qDebug() << "[MainWindow] Инициализация завершена";
}

MainWindow::~MainWindow()
{
    if (server && server->isRunning()) {
        qDebug() << "[MainWindow] Остановка сервера перед уничтожением";
        server->stop();
    }
    delete server;
    qDebug() << "[MainWindow] Объект MainWindow уничтожен";
}

void MainWindow::setupUI() {
    this->setMinimumSize(600, 400);
    qDebug() << "[MainWindow] Настройка UI";

    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(15);

    startButton = new QPushButton("Запустить сервер");
    stopButton = new QPushButton("Остановить сервер");
    stopButton->setEnabled(false);
    statusLabel = new QLabel("Сервер остановлен");
    statusLabel->setAlignment(Qt::AlignCenter);

    QString buttonStyle = R"(
        QPushButton {
            background-color: #4CAF50; /* Зеленый */
            color: white;
            border-radius: 8px;
            padding: 10px 20px;
            font-size: 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: #45a049;
        }
        QPushButton:disabled {
            background-color: #a5d6a7;
            color: #eee;
        }
    )";

    startButton->setStyleSheet(buttonStyle);
    stopButton->setStyleSheet(buttonStyle);

    QString labelStyle = R"(
        QLabel {
            font-size: 18px;
            font-family: "Arial";
            color: #333;
        }
    )";
    statusLabel->setStyleSheet(labelStyle);

    layout->addWidget(startButton, 0, Qt::AlignCenter);
    layout->addWidget(stopButton, 0, Qt::AlignCenter);
    layout->addWidget(statusLabel, 0, Qt::AlignCenter);

    setCentralWidget(centralWidget);

    // Подключения
    connect(startButton, &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    qDebug() << "[MainWindow] UI настроен";
}

void MainWindow::onStartClicked() {
    if (!server) {
        server = new UDPServer(12345);
        qDebug() << "[MainWindow] Создан объект сервера";
    }

    if (!server->isRunning()) {
        server->start();
        statusLabel->setText("Сервер запущен");
        startButton->setEnabled(false);
        stopButton->setEnabled(true);
        statusTimer->start(1000); // обновление статуса раз в секунду
        qDebug() << "[MainWindow] Сервер запущен";
    } else {
        qDebug() << "[MainWindow] Попытка запустить уже запущенный сервер";
    }
}

void MainWindow::onStopClicked() {
    if (server && server->isRunning()) {
        server->stop();
        statusLabel->setText("Сервер остановлен");
        startButton->setEnabled(true);
        stopButton->setEnabled(false);
        statusTimer->stop();
        qDebug() << "[MainWindow] Сервер остановлен";
    } else {
        qDebug() << "[MainWindow] Попытка остановить не запущенный сервер";
    }
}

void MainWindow::updateStatus() {
    if (server && server->isRunning()) {
        qDebug() << "[MainWindow] Обновление статуса: сервер запущен";
        statusLabel->setText("Сервер запущен");
    } else {
        qDebug() << "[MainWindow] Обновление статуса: сервер остановлен";
        statusLabel->setText("Сервер остановлен");
    }
}
