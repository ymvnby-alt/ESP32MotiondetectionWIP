#include "GraphWidget.h"

GraphWidget::GraphWidget(QWidget *parent)
    : QWidget(parent),
      m_plot(nullptr),
      m_wifiGraph(nullptr),
      m_bleGraph(nullptr),
      m_wifiThresholdLine(nullptr),
      m_bleThresholdLine(nullptr),
      m_timeWindowSeconds(60.0),
      m_bleAvailable(false) {
    setupPlot();
    m_clock.start();
}

void GraphWidget::setupPlot() {
    m_plot = new QCustomPlot(this);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_plot);
    setLayout(layout);

    m_plot->xAxis->setLabel("Time (s)");
    m_plot->yAxis->setLabel("Variance");
    m_plot->legend->setVisible(true);

    m_wifiGraph = m_plot->addGraph();
    m_wifiGraph->setName("WiFi CSI variance");
    m_wifiGraph->setPen(QPen(QColor(30, 144, 255)));

    m_bleGraph = m_plot->addGraph();
    m_bleGraph->setName("BLE RSSI variance");
    m_bleGraph->setPen(QPen(QColor(220, 90, 40)));

    m_wifiThresholdLine = new QCPItemStraightLine(m_plot);
    m_wifiThresholdLine->setPen(QPen(QColor(30, 144, 255), 1, Qt::DashLine));
    m_wifiThresholdLine->point1->setCoords(0, 0);
    m_wifiThresholdLine->point2->setCoords(1, 0);
    m_wifiThresholdLine->setVisible(false);

    m_bleThresholdLine = new QCPItemStraightLine(m_plot);
    m_bleThresholdLine->setPen(QPen(QColor(220, 90, 40), 1, Qt::DashLine));
    m_bleThresholdLine->point1->setCoords(0, 0);
    m_bleThresholdLine->point2->setCoords(1, 0);
    m_bleThresholdLine->setVisible(false);

    m_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
}

void GraphWidget::setTimeWindowSeconds(double seconds) {
    m_timeWindowSeconds = seconds;
}

void GraphWidget::trimOldData(QCPGraph *graph, double nowSecs) {
    double cutoff = nowSecs - m_timeWindowSeconds;
    graph->data()->removeBefore(cutoff);
}

void GraphWidget::addWifiSample(double variance) {
    double t = m_clock.elapsed() / 1000.0;
    m_wifiGraph->addData(t, variance);
    trimOldData(m_wifiGraph, t);

    // Keep the threshold line horizontal across the whole visible x-range
    m_wifiThresholdLine->point1->setCoords(t - m_timeWindowSeconds, m_wifiThresholdLine->point1->coords().y());
    m_wifiThresholdLine->point2->setCoords(t, m_wifiThresholdLine->point2->coords().y());

    m_plot->xAxis->setRange(t - m_timeWindowSeconds, t);
    m_plot->yAxis->rescale(true);
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void GraphWidget::addBleSample(double variance) {
    if (!m_bleAvailable) {
        return;
    }
    double t = m_clock.elapsed() / 1000.0;
    m_bleGraph->addData(t, variance);
    trimOldData(m_bleGraph, t);

    m_bleThresholdLine->point1->setCoords(t - m_timeWindowSeconds, m_bleThresholdLine->point1->coords().y());
    m_bleThresholdLine->point2->setCoords(t, m_bleThresholdLine->point2->coords().y());

    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void GraphWidget::setWifiThreshold(double threshold) {
    double t = m_clock.elapsed() / 1000.0;
    m_wifiThresholdLine->point1->setCoords(t - m_timeWindowSeconds, threshold);
    m_wifiThresholdLine->point2->setCoords(t, threshold);
    m_wifiThresholdLine->setVisible(true);
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void GraphWidget::setBleThreshold(double threshold, bool available) {
    m_bleAvailable = available;
    m_bleThresholdLine->setVisible(available);
    m_bleGraph->setVisible(available);
    if (available) {
        double t = m_clock.elapsed() / 1000.0;
        m_bleThresholdLine->point1->setCoords(t - m_timeWindowSeconds, threshold);
        m_bleThresholdLine->point2->setCoords(t, threshold);
    }
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}

void GraphWidget::clearData() {
    m_wifiGraph->data()->clear();
    m_bleGraph->data()->clear();
    m_clock.restart();
    m_plot->replot(QCustomPlot::rpQueuedReplot);
}
