#include "MotionDetector.h"
#include <QDateTime>
#include <cmath>

static const qint64 BLE_CONFIRM_WINDOW_MS = 3000;
static const int BLE_WINDOW_SIZE = 5;

MotionDetector::MotionDetector(QObject *parent)
    : QObject(parent),
      m_windowSize(10),
      m_debounceCount(3),
      m_wifiThreshold(10.0),
      m_bleThreshold(0.0),
      m_bleAvailable(false),
      m_consecutiveAbove(0),
      m_consecutiveBelow(0),
      m_confirmedMotion(false),
      m_lastState("idle"),
      m_lastBleMotionMs(-999999) {
}

void MotionDetector::setWifiThreshold(double threshold) {
    m_wifiThreshold = threshold;
}

void MotionDetector::setBleThreshold(double threshold, bool available) {
    m_bleThreshold = threshold;
    m_bleAvailable = available;
}

void MotionDetector::setWindowSize(int size) {
    m_windowSize = size;
}

void MotionDetector::setDebounceCount(int count) {
    m_debounceCount = count;
}

double MotionDetector::wifiThreshold() const {
    return m_wifiThreshold;
}

double MotionDetector::bleThreshold() const {
    return m_bleThreshold;
}

bool MotionDetector::bleAvailable() const {
    return m_bleAvailable;
}

void MotionDetector::reset() {
    m_csiWindow.clear();
    m_bleWindow.clear();
    m_consecutiveAbove = 0;
    m_consecutiveBelow = 0;
    m_confirmedMotion = false;
    m_lastState = "idle";
    m_lastBleMotionMs = -999999;
}

void MotionDetector::processCsi(CsiPacket packet) {
    double amplitude = computeCsiAmplitude(packet);

    m_csiWindow.append(amplitude);
    if (m_csiWindow.size() > m_windowSize) {
        m_csiWindow.removeFirst();
    }
    if (m_csiWindow.size() < m_windowSize) {
        return;
    }

    double variance = computeVarianceOf(m_csiWindow);
    emit wifiVarianceUpdated(variance, packet.rssi);

    bool rawOver = variance > m_wifiThreshold;
    if (rawOver) {
        m_consecutiveAbove++;
        m_consecutiveBelow = 0;
    } else {
        m_consecutiveBelow++;
        m_consecutiveAbove = 0;
    }

    emit debounceProgress(qMin(m_debounceCount, qMax(m_consecutiveAbove, m_consecutiveBelow)), m_debounceCount);

    if (!m_confirmedMotion && m_consecutiveAbove >= m_debounceCount) {
        m_confirmedMotion = true;
    } else if (m_confirmedMotion && m_consecutiveBelow >= m_debounceCount) {
        m_confirmedMotion = false;
    }

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    bool bleConfirms = (!m_bleAvailable) || (now - m_lastBleMotionMs < BLE_CONFIRM_WINDOW_MS);

    QString newState;
    if (!m_confirmedMotion) {
        newState = "idle";
    } else if (bleConfirms) {
        newState = "motion";
    } else {
        newState = "unconfirmed";
    }

    if (newState != m_lastState) {
        m_lastState = newState;
        emit stateChanged(newState);
    }
}

void MotionDetector::processBle(BlePacket packet) {
    m_bleWindow.append(static_cast<double>(packet.avgRssi));
    if (m_bleWindow.size() > BLE_WINDOW_SIZE) {
        m_bleWindow.removeFirst();
    }
    if (m_bleWindow.size() < 3) {
        return;
    }

    double variance = computeVarianceOf(m_bleWindow);
    emit bleVarianceUpdated(variance);

    if (m_bleAvailable && variance > m_bleThreshold) {
        m_lastBleMotionMs = QDateTime::currentMSecsSinceEpoch();
    }
}
