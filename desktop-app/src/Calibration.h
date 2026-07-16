#pragma once

#include <QObject>
#include <QVector>
#include <QTimer>
#include <QElapsedTimer>
#include "Packet.h"

class Calibration : public QObject {
    Q_OBJECT

public:
    enum class Phase {
        Idle,
        WarmupEmpty,
        ScanningEmpty,
        WarmupMotion,
        ScanningMotion,
        Done
    };
    Q_ENUM(Phase)

    explicit Calibration(QObject *parent = nullptr);

    void start(int windowSize, double warmupSeconds, double scanSeconds, double kSigma, double minThreshold);
    void processCsi(CsiPacket packet);
    void processBle(BlePacket packet);

signals:
    void phaseChanged(Phase phase, QString message);
    void progressUpdated(double remainingSeconds, int samplesCollected);
    void finished(double wifiThreshold, double bleThreshold, bool bleAvailable);

private slots:
    void onTick();

private:
    static double meanOf(const QVector<double> &values);
    static double stdDevOf(const QVector<double> &values, double mean);
    void finishCalibration();

    QTimer m_ticker;
    QElapsedTimer m_phaseTimer;
    Phase m_phase;

    int m_windowSize;
    double m_warmupSeconds;
    double m_scanSeconds;
    double m_kSigma;
    double m_minThreshold;

    QVector<double> m_csiWindow;
    QVector<double> m_bleWindow;

    QVector<double> m_emptyCsiSamples;
    QVector<double> m_motionCsiSamples;
    QVector<double> m_emptyBleSamples;
    QVector<double> m_motionBleSamples;

    double computeAmplitude(const CsiPacket &packet) const;
    static double computeVariance(const QVector<double> &values);
};
