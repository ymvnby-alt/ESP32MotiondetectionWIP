#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <QString>
#include <QVector>

enum class PacketType : uint8_t {
    Csi = 0x01,
    Ble = 0x02
};

struct CsiPacket {
    int8_t rssi;
    std::vector<int8_t> data;
};

struct BlePacket {
    int16_t avgRssi;
    uint8_t deviceCount;
};

struct NetworkEntry {
    int index;
    QString ssid;
    int channel;
    int rssi;
};

inline double computeCsiAmplitude(const CsiPacket &packet) {
    int count = static_cast<int>(packet.data.size());
    int pairCount = count / 2;
    if (pairCount == 0) {
        return 0.0;
    }
    double sum = 0.0;
    for (int i = 0; i < pairCount; i++) {
        double iVal = static_cast<double>(packet.data[i * 2]);
        double qVal = static_cast<double>(packet.data[i * 2 + 1]);
        sum += std::sqrt(iVal * iVal + qVal * qVal);
    }
    return sum / pairCount;
}

inline double computeVarianceOf(const QVector<double> &values) {
    if (values.isEmpty()) {
        return 0.0;
    }
    double mean = 0.0;
    for (double v : values) {
        mean += v;
    }
    mean /= values.size();

    double sumSq = 0.0;
    for (double v : values) {
        double diff = v - mean;
        sumSq += diff * diff;
    }
    return sumSq / values.size();
}

