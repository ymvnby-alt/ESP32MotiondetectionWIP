#pragma once

#include <QObject>
#include <QVector>
#include "Packet.h"

class MotionDetector : public QObject {
    Q_OBJECT

public:
    explicit MotionDetector(QObject *parent = nullptr);

    void setWifiThreshold(double threshold);
    void setBleThreshold(double threshold, bool available);
    void setWindowSize(int size);
    void setDebounceCount(int count);

    double wifiThreshold() const;
    double bleThreshold() const;
    bool bleAvailable() const;

public slots:
    void processCsi(CsiPacket packet);
    void processBle(BlePacket packet);
    void reset();

signals:
    void wifiVarianceUpdated(double variance, int rssi);
    void bleVarianceUpdated(double variance);
    void stateChanged(QString state);
    void debounceProgress(int current, int total);

private:
    QVector<double> m_csiWindow;
    QVector<double> m_bleWindow;
    int m_windowSize;
    int m_debounceCount;
    double m_wifiThreshold;
    double m_bleThreshold;
    bool m_bleAvailable;
    int m_consecutiveAbove;
    int m_consecutiveBelow;
    bool m_confirmedMotion;
    QString m_lastState;
    qint64 m_lastBleMotionMs;
};
