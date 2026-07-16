#pragma once

#include <QObject>
#include <QSerialPort>
#include <QByteArray>
#include "Packet.h"

class SerialWorker : public QObject {
    Q_OBJECT

public:
    explicit SerialWorker(QObject *parent = nullptr);
    ~SerialWorker();

public slots:
    void openPort(const QString &portName, qint32 baudRate);
    void closePort();
    void sendCommand(const QString &command);
    void sendMotionState(bool motionConfirmed);

signals:
    void csiReceived(CsiPacket packet);
    void bleReceived(BlePacket packet);
    void textLineReceived(QString line);
    void errorOccurred(QString message);
    void portOpened();
    void portClosed();

private slots:
    void handleReadyRead();
    void handleErrorOccurred(QSerialPort::SerialPortError error);

private:
    void processBuffer();

    QSerialPort *m_port;
    QByteArray m_buffer;
};
