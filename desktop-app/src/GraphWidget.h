#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QElapsedTimer>
#include "qcustomplot.h"

class GraphWidget : public QWidget {
    Q_OBJECT

public:
    explicit GraphWidget(QWidget *parent = nullptr);

public slots:
    void addWifiSample(double variance);
    void addBleSample(double variance);
    void setWifiThreshold(double threshold);
    void setBleThreshold(double threshold, bool available);
    void clearData();
    void setTimeWindowSeconds(double seconds);

private:
    void setupPlot();
    void trimOldData(QCPGraph *graph, double nowSecs);

    QCustomPlot *m_plot;
    QCPGraph *m_wifiGraph;
    QCPGraph *m_bleGraph;
    QCPItemStraightLine *m_wifiThresholdLine;
    QCPItemStraightLine *m_bleThresholdLine;

    QElapsedTimer m_clock;
    double m_timeWindowSeconds;
    bool m_bleAvailable;
};
