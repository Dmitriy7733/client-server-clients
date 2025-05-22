//mainwindow.cpp
#include "mainwindow.h"

#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 1
#define SAMPLE_SIZE 2 // 16-bit

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    serverIP("127.0.0.1"),
    serverPort(12345),
    connectionConfirmed(false),
    bufferMutex(),
    isTalking(false),
    udpSocket(new QUdpSocket(this)),
    recvTimer(new QTimer(this)),
    connected(false),
    inputStream(nullptr),
    outputStream(nullptr)
{
    this->setMinimumSize(600, 400);

    udpSocket = new QUdpSocket(this);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // IP
    QHBoxLayout *ipLayout = new QHBoxLayout();
    ipLayout->addWidget(new QLabel("IP:"));
    ipLineEdit = new QLineEdit("127.0.0.1");
    ipLayout->addWidget(ipLineEdit);
    layout->addLayout(ipLayout);

    // Порт
    QHBoxLayout *portLayout = new QHBoxLayout();
    portLayout->addWidget(new QLabel("Port:"));
    portLineEdit = new QLineEdit("12345");
    portLayout->addWidget(portLineEdit);
    layout->addLayout(portLayout);

    // Кнопка подключения
    connectButton = new QPushButton("Подключение");
    layout->addWidget(connectButton);

    // Кнопка говорить
    talkButton = new QPushButton("Говорить");
    talkButton->setCheckable(true);
    layout->addWidget(talkButton);

    // --- Устройства PortAudio ---
    inputDeviceComboBox = new QComboBox();
    outputDeviceComboBox = new QComboBox();

    // Инициализация PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qDebug() << "PortAudio init error:" << Pa_GetErrorText(err);
    } else {
        logAvailableDevices();
        fillDeviceLists();
    }

    preferredSampleRates = {48000.0, 44100.0, 32000.0, 16000.0, 8000.0};
    //fillDeviceLists();

    // --- Разметка для выбора устройств ---
    QHBoxLayout *inputDeviceLayout = new QHBoxLayout();
    inputDeviceLayout->addWidget(new QLabel("Входное устройство:"));
    inputDeviceLayout->addWidget(inputDeviceComboBox);
    layout->addLayout(inputDeviceLayout);

    QHBoxLayout *outputDeviceLayout = new QHBoxLayout();
    outputDeviceLayout->addWidget(new QLabel("Выходное устройство:"));
    outputDeviceLayout->addWidget(outputDeviceComboBox);
    layout->addLayout(outputDeviceLayout);

    // --- Сигналы ---
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(talkButton, &QPushButton::pressed, this, &MainWindow::onTalkPressed);
    connect(talkButton, &QPushButton::released, this, &MainWindow::onTalkReleased);
    connect(recvTimer, &QTimer::timeout, this, &MainWindow::onDataReceived);

    // Перезапуск аудио при смене устройств
    connect(inputDeviceComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::restartAudio);
    connect(outputDeviceComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::restartAudio);

    // Изначально не подключены
    connected = false;
}

MainWindow::~MainWindow()
{
    stopAudio();
    Pa_Terminate();
}

double MainWindow::getSupportedSampleRateWithLogging(int deviceId, const std::vector<double>& preferredSampleRates, bool isInput) {
    auto deviceInfo = Pa_GetDeviceInfo(deviceId);
    if (!deviceInfo) {
        qDebug() << "Неверный идентификатор устройства" << deviceId;
        return 0;
    }
    for (double rate : preferredSampleRates) {
        PaStreamParameters params;
        if (isInput) {
            params.device = deviceId;
            params.channelCount = NUM_CHANNELS;
            params.sampleFormat = paInt16;
            params.hostApiSpecificStreamInfo = nullptr;
            if (Pa_IsFormatSupported(&params, nullptr, rate) == paNoError) {
                qDebug() << "Устройство" << QString::fromUtf8(deviceInfo->name) << "поддерживает частоту" << rate;
                return rate;
            }
        } else {
            params.device = deviceId;
            params.channelCount = NUM_CHANNELS;
            params.sampleFormat = paInt16;
            params.hostApiSpecificStreamInfo = nullptr;
            if (Pa_IsFormatSupported(nullptr, &params, rate) == paNoError) {
                qDebug() << "Устройство" << QString::fromUtf8(deviceInfo->name) << "поддерживает частоту" << rate;
                return rate;
            }
        }
    }
    qDebug() << "Устройство" << QString::fromUtf8(deviceInfo->name) << "не поддерживает предпочтительные форматы.";
    return 0;
}

void MainWindow::logAvailableDevices()
{
    int numDevices = Pa_GetDeviceCount();
    qDebug() << "Обнаружено устройств PortAudio:" << numDevices;
    for (int i = 0; i < numDevices; ++i) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (info) {
            QString name = QString::fromUtf8(info->name);
            qDebug() << "Device" << i << ":" << name
                     << "MaxInputChannels:" << info->maxInputChannels
                     << "MaxOutputChannels:" << info->maxOutputChannels
                     << "DefaultSampleRate:" << info->defaultSampleRate;
        }
    }
}

void MainWindow::fillDeviceLists()
{
    inputDeviceComboBox->clear();
    outputDeviceComboBox->clear();

    int deviceCount = Pa_GetDeviceCount();
    if (deviceCount < 0) {
        qDebug() << "Ошибка получения количества устройств:" << Pa_GetErrorText(deviceCount);
        return;
    }

    for (int i = 0; i < deviceCount; ++i) {
        const PaDeviceInfo *info = Pa_GetDeviceInfo(i);
        if (!info) continue;
        QString name = QString::fromUtf8(info->name);
        if (info->maxInputChannels > 0) {
            inputDeviceComboBox->addItem(name, i);
        }
        if (info->maxOutputChannels > 0) {
            outputDeviceComboBox->addItem(name, i);
        }
    }
}

void MainWindow::onDataReceived() {
    while (udpSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(int(udpSocket->pendingDatagramSize()));
        QHostAddress sender;
        quint16 senderPort;
        qDebug() << "Прием датаграммы размером:" << datagram.size();
        udpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        qDebug() << "Received datagram of size" << datagram.size() << "от" << sender.toString();

        QMutexLocker locker(&bufferMutex);
        buffer.append(datagram);
        qDebug() << "Общий размер буфера воспроизведения:" << buffer.size() << "байт";
    }
}

void MainWindow::onTalkPressed()
{
    if (!isTalking) {
        isTalking = true;
        qDebug() << "Начинаем говорить";

        int inputDeviceId = inputDeviceComboBox->currentData().toInt();
        int outputDeviceId = outputDeviceComboBox->currentData().toInt();

        int result = startAudio(inputDeviceId, outputDeviceId);
        if (result != 0) {
            qDebug() << "Ошибка запуска аудио";
            isTalking = false;
        }
    }
}
void MainWindow::onTalkReleased()
{
    if (isTalking) {
        isTalking = false;
        qDebug() << "Заканчиваем говорить";
        stopAudio();
    }
}

PaStreamParameters MainWindow::getDefaultInputParameters()
{
    PaStreamParameters inputParams;
    int device = Pa_GetDefaultInputDevice();
    if (device == paNoDevice) {
        qDebug() << "Нет устройства для записи.";
        return inputParams;
    }
    auto deviceInfo = Pa_GetDeviceInfo(device);
    inputParams.device = device;
    inputParams.channelCount = NUM_CHANNELS;
    inputParams.sampleFormat = paInt16;
    inputParams.suggestedLatency = deviceInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;
    return inputParams;
}

PaStreamParameters MainWindow::getDefaultOutputParameters()
{
    PaStreamParameters outputParams;
    int device = Pa_GetDefaultOutputDevice();
    if (device == paNoDevice) {
        qDebug() << "Нет устройства для воспроизведения.";
        return outputParams;
    }
    auto deviceInfo = Pa_GetDeviceInfo(device);
    outputParams.device = device;
    outputParams.channelCount = NUM_CHANNELS;
    outputParams.sampleFormat = paInt16;
    outputParams.suggestedLatency = deviceInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;
    return outputParams;
}

int MainWindow::startAudio(int inputDeviceId, int outputDeviceId)
{
    stopAudio();

    auto inputDeviceInfo = Pa_GetDeviceInfo(inputDeviceId);
    auto outputDeviceInfo = Pa_GetDeviceInfo(outputDeviceId);
    if (!inputDeviceInfo || !outputDeviceInfo) {
        qDebug() << "Некорректные устройства";
        return -1;
    }

    // Определение подходящей частоты
    std::vector<double> preferredSampleRates = {48000.0, 44100.0, 32000.0, 16000.0, 8000.0};
    double sampleRateInput = getSupportedSampleRateWithLogging(inputDeviceId, preferredSampleRates, true);
    double sampleRateOutput = getSupportedSampleRateWithLogging(outputDeviceId, preferredSampleRates, false);

    if (sampleRateInput == 0 || sampleRateOutput == 0) {
        qDebug() << "Не поддерживаются предпочтительные форматы.";
        return -1;
    }

    double selectedSampleRate = sampleRateInput; // или логика выбора

    // Настройка входных параметров
    PaStreamParameters inputParams;
    inputParams.device = inputDeviceId;
    inputParams.channelCount = NUM_CHANNELS;
    inputParams.sampleFormat = paInt16;
    inputParams.suggestedLatency = inputDeviceInfo->defaultLowInputLatency;
    inputParams.hostApiSpecificStreamInfo = nullptr;

    // Настройка выходных параметров
    PaStreamParameters outputParams;
    outputParams.device = outputDeviceId;
    outputParams.channelCount = NUM_CHANNELS;
    outputParams.sampleFormat = paInt16;
    outputParams.suggestedLatency = outputDeviceInfo->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    // Открытие входного потока
    PaError err = Pa_OpenStream(
        &inputStream,
        &inputParams,
        nullptr,
        selectedSampleRate,
        FRAMES_PER_BUFFER,
        paClipOff,
        &MainWindow::recordCallback,
        this
        );
    if (err != paNoError) {
        qDebug() << "Ошибка открытия входного потока:" << Pa_GetErrorText(err);
        return -1;
    }

    // Открытие выходного потока
    err = Pa_OpenStream(
        &outputStream,
        nullptr,
        &outputParams,
        selectedSampleRate,
        FRAMES_PER_BUFFER,
        paClipOff,
        &MainWindow::playCallback,
        this
        );
    if (err != paNoError) {
        qDebug() << "Ошибка открытия выходного потока:" << Pa_GetErrorText(err);
        Pa_CloseStream(inputStream);
        inputStream = nullptr;
        return -1;
    }

    // Запуск потоков
    err = Pa_StartStream(inputStream);
    if (err != paNoError) {
        qDebug() << "Ошибка запуска входного потока:" << Pa_GetErrorText(err);
        Pa_CloseStream(inputStream);
        inputStream = nullptr;
        Pa_CloseStream(outputStream);
        outputStream = nullptr;
        return -1;
    }

    err = Pa_StartStream(outputStream);
    if (err != paNoError) {
        qDebug() << "Ошибка запуска выходного потока:" << Pa_GetErrorText(err);
        Pa_StopStream(inputStream);
        Pa_CloseStream(inputStream);
        inputStream = nullptr;
        Pa_CloseStream(outputStream);
        outputStream = nullptr;
        return -1;
    }

    qDebug() << "Аудио запущено с частотой" << selectedSampleRate;
    return 0;
}

void MainWindow::stopAudio() {
    if (inputStream) {
        Pa_StopStream(inputStream);
        Pa_CloseStream(inputStream);
        inputStream = nullptr;
        qDebug() << "Входной поток остановлен.";
    }
    if (outputStream) {
        Pa_StopStream(outputStream);
        Pa_CloseStream(outputStream);
        outputStream = nullptr;
        qDebug() << "Выходной поток остановлен.";
    }
}

// Коллбэки
int MainWindow::recordCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount,
                               const PaStreamCallbackTimeInfo *timeInfo,
                               PaStreamCallbackFlags statusFlags,
                               void *userData) {
    Q_UNUSED(outputBuffer);
    (void)timeInfo;
    (void)statusFlags;

    MainWindow *self = static_cast<MainWindow*>(userData);
    const short *in = static_cast<const short*>(inputBuffer);
    size_t dataSize = frameCount * NUM_CHANNELS;

    qDebug() << "recordCallback вызван. frameCount:" << frameCount;

    if (!self->isTalking || inputBuffer == nullptr) {
        return paContinue;
    }

    QByteArray data(reinterpret_cast<const char*>(in), dataSize * SAMPLE_SIZE);

    if (self->connected) {
        self->udpSocket->writeDatagram(data, QHostAddress(self->serverIP), self->serverPort);
        qDebug() << "Отправлено" << data.size() << "байт данных.";
    } else {
        qDebug() << "Не подключено или подтверждение отсутствует.";
    }

    return paContinue;
}

int MainWindow::playCallback(const void *inputBuffer, void *outputBuffer, unsigned long frameCount,
                             const PaStreamCallbackTimeInfo *timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData) {
    Q_UNUSED(inputBuffer);
    (void)timeInfo;
    (void)statusFlags;

    MainWindow *self = static_cast<MainWindow*>(userData);
    short *out = static_cast<short*>(outputBuffer);
    size_t bytesNeeded = frameCount * NUM_CHANNELS * SAMPLE_SIZE;

    QMutexLocker locker(&self->bufferMutex);
    size_t available = self->buffer.size();

    if (available >= bytesNeeded) {
        memcpy(out, self->buffer.data(), bytesNeeded);
        self->buffer.remove(0, bytesNeeded);
        qDebug() << "Данных достаточно, воспроизводим.";
    } else {
        // Недостаточно данных — заполняем нулями
        memset(out, 0, bytesNeeded);
        qDebug() << "Недостаточно данных, затираем нулями.";
    }

    return paContinue;
}

int MainWindow::onConnectClicked() {
    if (!connected) {
        // Получение IP и порта из интерфейса
        serverIP = ipLineEdit->text();
        serverPort = static_cast<unsigned short>(portLineEdit->text().toUShort());
        qDebug() << "Попытка подключиться к" << serverIP << ":" << serverPort;

        // Проверка корректности IP и порта
        QHostAddress address;
        if (!address.setAddress(serverIP)) {
            qDebug() << "Некорректный IP-адрес:" << serverIP;
            return -1;
        }
        if (serverPort == 0) {
            qDebug() << "Некорректный порт:" << serverPort;
            return -1;
        }

        // Удаление старого сокета, если есть
        if (udpSocket) {
            if (udpSocket->isOpen()) {
                udpSocket->close();
            }
            delete udpSocket;
            udpSocket = nullptr;
        }

        // Создаем новый UDP сокет
        udpSocket = new QUdpSocket(this);

        // Привязка к любому свободному порту
        if (!udpSocket->bind(QHostAddress::Any, 0)) {
            qDebug() << "Не удалось привязать сокет:" << udpSocket->errorString();
            delete udpSocket;
            udpSocket = nullptr;
            return -1;
        }

        // Отправляем сообщение CONNECT
        QByteArray connectMsg = "CONNECT";
        qint64 bytesSent = udpSocket->writeDatagram(connectMsg, address, serverPort);
        if (bytesSent == -1) {
            qDebug() << "Ошибка отправки CONNECT:" << udpSocket->errorString();
            udpSocket->close();
            delete udpSocket;
            udpSocket = nullptr;
            return -1;
        } else {
            qDebug() << "Отправлено" << bytesSent << "байт CONNECT.";
        }

        // Получение выбранных устройств для аудио
        int inputDeviceId = inputDeviceComboBox->currentData().toInt();
        int outputDeviceId = outputDeviceComboBox->currentData().toInt();

        // Запуск аудио
        int result = startAudio(inputDeviceId, outputDeviceId);
        if (result != 0) {
            qDebug() << "Ошибка запуска аудио";
            return -1;
        }

        // Установка флага и обновление интерфейса
        connected = true;
        connectButton->setText("Отключиться");
        recvTimer->start(10); // Таймер для чтения данных
        qDebug() << "Клиент подключен к" << serverIP << ":" << serverPort;
        return 0;
    } else {
        stopAudio();

        if (udpSocket) {
            // Отправляем DISCONNECT перед закрытием
            QHostAddress address;
            if (!address.setAddress(serverIP)) {
                qDebug() << "Некорректный IP-адрес при отключении:" << serverIP;
            } else {
                QByteArray disconnectMsg = "DISCONNECT";
                qint64 bytesSent = udpSocket->writeDatagram(disconnectMsg, address, serverPort);
                if (bytesSent == -1) {
                    qDebug() << "Ошибка отправки DISCONNECT:" << udpSocket->errorString();
                } else {
                    qDebug() << "Отправлено" << bytesSent << "байт DISCONNECT.";
                }
            }
            // Закрываем и удаляем сокет
            udpSocket->close();
            delete udpSocket;
            udpSocket = nullptr;
        }

        // Обновляем состояние
        connected = false;
        connectButton->setText("Подключение");
        recvTimer->stop();
        qDebug() << "Клиент отключен";
        return 0;
    }
}
void MainWindow::restartAudio()
{
    // Остановить текущие потоки
    stopAudio();

    // Получить выбранные устройства
    int inputDeviceId = inputDeviceComboBox->currentData().toInt();
    int outputDeviceId = outputDeviceComboBox->currentData().toInt();

    // Запустить заново
    startAudio(inputDeviceId, outputDeviceId);
}
