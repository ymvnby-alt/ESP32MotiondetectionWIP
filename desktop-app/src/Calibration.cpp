#include "Calibration.h"
#include <cmath>
#include <QtGlobal>

Calibration::Calibration(QObject *parent)
    : QObject(parent),
      m_phase(Phase::Idle),
      m_windowSize(10),
      m_warmupSeconds(10.0),
      m_scanSeconds(30.0),
      m_kSigma(5.0),
      m_minThreshold(10.0) {
    m_ticker.setInterval(100);
    connect(&m_ticker, &QTimer::timeout, this, &Calibration::onTick);
}

void Calibration::start(int windowSize, double warmupSeconds, double scanSeconds, double kSigma, double minThreshold) {
    m_windowSize = windowSize;
    m_warmupSeconds = warmupSeconds;
    m_scanSeconds = scanSeconds;
    m_kSigma = kSigma;
    m_minThreshold = minThreshold;

    m_csiWindow.clear();
    m_bleWindow.clear();
    m_emptyCsiSamples.clear();
    m_motionCsiSamples.clear();
    m_emptyBleSamples.clear();
    m_motionBleSamples.clear();

    m_phase = Phase::WarmupEmpty;
    m_phaseTimer.restart();
    m_ticker.start();

    emit phaseChanged(Phase::WarmupEmpty, "Leave the room / stay away from the antenna...");
}

void Calibration::processCsi(CsiPacket packet) {
    if (m_phase == Phase::Idle || m_phase == Phase::Done) {
        return;
    }

    double amplitude = computeAmplitude(packet);
    m_csiWindow.append(amplitude);
    if (m_csiWindow.size() > m_windowSize) {
        m_csiWindow.removeFirst();
    }
    if (m_csiWindow.size() < m_windowSize) {
        return;
    }

    double variance = computeVariance(m_csiWindow);

    if (m_phase == Phase::ScanningEmpty) {
        m_emptyCsiSamples.append(variance);
    } else if (m_phase == Phase::ScanningMotion) {
        m_motionCsiSamples.append(variance);
    }
}

void Calibration::processBle(BlePacket packet) {
    if (m_phase == Phase::Idle || m_phase == Phase::Done) {
        return;
    }

    m_bleWindow.append(static_cast<double>(packet.avgRssi));
    if (m_bleWindow.size() > 5) {
        m_bleWindow.removeFirst();
    }
    if (m_bleWindow.size() < 3) {
        return;
    }

    double variance = computeVariance(m_bleWindow);

    if (m_phase == Phase::ScanningEmpty) {
        m_emptyBleSamples.append(variance);
    } else if (m_phase == Phase::ScanningMotion) {
        m_motionBleSamples.append(variance);
    }
}

void Calibration::onTick() {
    double elapsed = m_phaseTimer.elapsed() / 1000.0;

    switch (m_phase) {
        case Phase::WarmupEmpty: {
            double remaining = m_warmupSeconds - elapsed;
            emit progressUpdated(qMax(0.0, remaining), 0);
            if (remaining <= 0) {
                m_phase = Phase::ScanningEmpty;
                m_phaseTimer.restart();
                emit phaseChanged(Phase::ScanningEmpty, "Measuring empty-room baseline...");
            }
            break;
        }
        case Phase::ScanningEmpty: {
            double remaining = m_scanSeconds - elapsed;
            emit progressUpdated(qMax(0.0, remaining), m_emptyCsiSamples.size());
            if (remaining <= 0) {
                m_phase = Phase::WarmupMotion;
                m_phaseTimer.restart();
                emit phaseChanged(Phase::WarmupMotion, "Now go stand/walk in front of the antenna...");
            }
            break;
        }
        case Phase::WarmupMotion: {
            double remaining = m_warmupSeconds - elapsed;
            emit progressUpdated(qMax(0.0, remaining), 0);
            if (remaining <= 0) {
                m_phase = Phase::ScanningMotion;
                m_phaseTimer.restart();
                emit phaseChanged(Phase::ScanningMotion, "Measuring movement...");
            }
            break;
        }
        case Phase::ScanningMotion: {
            double remaining = m_scanSeconds - elapsed;
            emit progressUpdated(qMax(0.0, remaining), m_motionCsiSamples.size());
            if (remaining <= 0) {
                m_ticker.stop();
                m_phase = Phase::Done;
                finishCalibration();
            }
            break;
        }
        default:
            break;
    }
}

double Calibration::meanOf(const QVector<double> &values) {
    if (values.isEmpty()) {
        return 0.0;
    }
    double sum = 0.0;
    for (double v : values) {
        sum += v;
    }
    return sum / values.size();
}

double Calibration::stdDevOf(const QVector<double> &values, double mean) {
    if (values.isEmpty()) {
        return 0.0;
    }
    double sumSq = 0.0;
    for (double v : values) {
        double diff = v - mean;
        sumSq += diff * diff;
    }
    return std::sqrt(sumSq / values.size());
}

void Calibration::finishCalibration() {
    double emptyMean = meanOf(m_emptyCsiSamples);
    double emptyStd = stdDevOf(m_emptyCsiSamples, emptyMean);
    double motionMean = meanOf(m_motionCsiSamples);
    double noiseCeiling = emptyMean + m_kSigma * emptyStd;

    double wifiThreshold;
    QString message;
    if (motionMean <= noiseCeiling) {
        wifiThreshold = qMax(m_minThreshold, noiseCeiling);
        message = "Warning: movement variance wasn't clearly separated from empty-room noise. Consider re-scanning.";
    } else {
        wifiThreshold = qMax(m_minThreshold, (noiseCeiling + motionMean) / 2.0);
        message = "Calibration complete.";
    }

    bool bleAvailable = !m_emptyBleSamples.isEmpty() && !m_motionBleSamples.isEmpty();
    double bleThreshold = 0.0;

    if (bleAvailable) {
        double bleEmptyMean = meanOf(m_emptyBleSamples);
        double bleEmptyStd = stdDevOf(m_emptyBleSamples, bleEmptyMean);
        double bleMotionMean = meanOf(m_motionBleSamples);
        double bleNoiseCeiling = bleEmptyMean + m_kSigma * bleEmptyStd;

        if (bleMotionMean <= bleNoiseCeiling) {
            bleThreshold = qMax(1.0, bleNoiseCeiling);
        } else {
            bleThreshold = qMax(1.0, (bleNoiseCeiling + bleMotionMean) / 2.0);
        }
    }

    emit phaseChanged(Phase::Done, message);
    emit finished(wifiThreshold, bleThreshold, bleAvailable);
}

double Calibration::computeAmplitude(const CsiPacket &packet) const {
    return computeCsiAmplitude(packet);
}

double Calibration::computeVariance(const QVector<double> &values) {
    return computeVarianceOf(values);
}
