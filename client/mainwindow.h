//mainwindow.h

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTimer>
#include <QUdpSocket>
#include <QComboBox>
#include <QMutex>
#include <vector>
#include <portaudio.h>
#include <pa_asio.h>

class MainWindow : public QMainWindow
{
    Q_OBJECT

private:
    // Переменные
    QString serverIP;
    quint16 serverPort;
    bool connectionConfirmed;
    QMutex bufferMutex;
    bool isTalking;
    QByteArray buffer;
    QLineEdit *ipLineEdit;
    QLineEdit *portLineEdit;
    QComboBox *inputDeviceComboBox;
    QComboBox *outputDeviceComboBox;
    QPushButton *connectButton;
    QPushButton *talkButton;
    QUdpSocket *udpSocket;
    QTimer *recvTimer;

    bool connected = false;
    PaStream *inputStream = nullptr;
    PaStream *outputStream = nullptr;
    unsigned short serverPortLocal;
    std::vector<double> preferredSampleRates;

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    int onConnectClicked();
    void onTalkPressed();
    void onTalkReleased();
    void onDataReceived();
    void restartAudio();

    int startAudio(int inputDeviceId, int outputDeviceId);
    void stopAudio();
    double getSupportedSampleRateWithLogging(int deviceId, const std::vector<double>& preferredSampleRates, bool isInput);
    PaStreamParameters getDefaultInputParameters();
    PaStreamParameters getDefaultOutputParameters();
    void logAvailableDevices();
    void fillDeviceLists();

    static int recordCallback(const void *inputBuffer, void *outputBuffer,
                              unsigned long frameCount,
                              const PaStreamCallbackTimeInfo *timeInfo,
                              PaStreamCallbackFlags statusFlags,
                              void *userData);
    static int playCallback(const void *inputBuffer, void *outputBuffer,
                            unsigned long frameCount,
                            const PaStreamCallbackTimeInfo *timeInfo,
                            PaStreamCallbackFlags statusFlags,
                            void *userData);
};
#endif // MAINWINDOW_H

