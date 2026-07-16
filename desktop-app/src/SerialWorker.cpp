#include "SerialWorker.h"

SerialWorker::SerialWorker(QObject *parent)
    : QObject(parent), m_port(nullptr) {
}

SerialWorker::~SerialWorker() {
    closePort();
}

void SerialWorker::openPort(const QString &portName, qint32 baudRate) {
    if (m_port) {
        closePort();
    }

    m_port = new QSerialPort(this);
    m_port->setPortName(portName);
    m_port->setBaudRate(baudRate);
    m_port->setDataBits(QSerialPort::Data8);
    m_port->setParity(QSerialPort::NoParity);
    m_port->setStopBits(QSerialPort::OneStop);
    m_port->setFlowControl(QSerialPort::NoFlowControl);

    connect(m_port, &QSerialPort::readyRead, this, &SerialWorker::handleReadyRead);
    connect(m_port, &QSerialPort::errorOccurred, this, &SerialWorker::handleErrorOccurred);

    if (!m_port->open(QIODevice::ReadWrite)) {
        emit errorOccurred(m_port->errorString());
        delete m_port;
        m_port = nullptr;
        return;
    }

    m_buffer.clear();
    emit portOpened();
}

void SerialWorker::closePort() {
    if (m_port) {
        if (m_port->isOpen()) {
            m_port->close();
        }
        delete m_port;
        m_port = nullptr;
        emit portClosed();
    }
}

void SerialWorker::sendCommand(const QString &command) {
    if (m_port && m_port->isOpen()) {
        QByteArray data = command.toUtf8();
        data.append('\n');
        m_port->write(data);
    }
}

void SerialWorker::sendMotionState(bool motionConfirmed) {
    if (m_port && m_port->isOpen()) {
        m_port->write(motionConfirmed ? "M\n" : "N\n");
    }
}

void SerialWorker::handleErrorOccurred(QSerialPort::SerialPortError error) {
    if (error != QSerialPort::NoError) {
        emit errorOccurred(m_port ? m_port->errorString() : QString("Unknown serial error"));
    }
}

void SerialWorker::handleReadyRead() {
    if (!m_port) return;
    m_buffer.append(m_port->readAll());
    processBuffer();
}

void SerialWorker::processBuffer() {
    while (true) {
        if (m_buffer.isEmpty()) {
            break;
        }

        unsigned char first = static_cast<unsigned char>(m_buffer.at(0));

        if (first == 0xAA) {
            if (m_buffer.size() < 2) {
                break;
            }
            unsigned char second = static_cast<unsigned char>(m_buffer.at(1));
            if (second != 0x55) {
                m_buffer.remove(0, 1);
                continue;
            }
            if (m_buffer.size() < 5) {
                break;
            }

            uint8_t type = static_cast<unsigned char>(m_buffer.at(2));
            uint8_t lenLow = static_cast<unsigned char>(m_buffer.at(3));
            uint8_t lenHigh = static_cast<unsigned char>(m_buffer.at(4));
            uint16_t length = static_cast<uint16_t>(lenLow) | (static_cast<uint16_t>(lenHigh) << 8);

            int totalFrameSize = 5 + length + 1;
            if (m_buffer.size() < totalFrameSize) {
                break;
            }

            QByteArray payload = m_buffer.mid(5, length);
            unsigned char receivedChecksum = static_cast<unsigned char>(m_buffer.at(5 + length));

            unsigned char computedChecksum = 0;
            for (int i = 0; i < payload.size(); i++) {
                computedChecksum ^= static_cast<unsigned char>(payload.at(i));
            }

            if (computedChecksum == receivedChecksum) {
                if (type == static_cast<uint8_t>(PacketType::Csi) && payload.size() >= 1) {
                    CsiPacket packet;
                    packet.rssi = static_cast<int8_t>(payload.at(0));
                    packet.data.reserve(payload.size() - 1);
                    for (int i = 1; i < payload.size(); i++) {
                        packet.data.push_back(static_cast<int8_t>(payload.at(i)));
                    }
                    emit csiReceived(packet);
                } else if (type == static_cast<uint8_t>(PacketType::Ble) && payload.size() >= 3) {
                    BlePacket packet;
                    int16_t rssi = static_cast<int16_t>(
                        static_cast<uint8_t>(payload.at(0)) |
                        (static_cast<uint8_t>(payload.at(1)) << 8)
                    );
                    packet.avgRssi = rssi;
                    packet.deviceCount = static_cast<uint8_t>(payload.at(2));
                    emit bleReceived(packet);
                }
            }

            m_buffer.remove(0, totalFrameSize);
            continue;
        }

        int newlineIndex = m_buffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        QByteArray lineBytes = m_buffer.left(newlineIndex);
        if (lineBytes.endsWith('\r')) {
            lineBytes.chop(1);
        }
        emit textLineReceived(QString::fromUtf8(lineBytes));
        m_buffer.remove(0, newlineIndex + 1);
    }
}
