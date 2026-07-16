#pragma once

#include <QMainWindow>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QProgressBar>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QVector>

#include "SerialWorker.h"
#include "MotionDetector.h"
#include "Calibration.h"
#include "GraphWidget.h"
#include "Packet.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onRefreshPortsClicked();
    void onConnectClicked();
    void onCalibrateClicked();
    void onScanNetworksClicked();
    void onConnectNetworkClicked();
    void onSniffNetworkClicked();

    void onPortOpened();
    void onPortClosed();
    void onSerialError(const QString &message);

    void onCsiReceived(CsiPacket packet);
    void onBleReceived(BlePacket packet);
    void onTextLineReceived(const QString &line);

    void onWifiVarianceUpdated(double variance, int rssi);
    void onBleVarianceUpdated(double variance);
    void onMotionStateChanged(const QString &state);
    void onDebounceProgress(int current, int total);

    void onCalibrationPhaseChanged(Calibration::Phase phase, QString message);
    void onCalibrationProgress(double remainingSeconds, int samplesCollected);
    void onCalibrationFinished(double wifiThreshold, double bleThreshold, bool bleAvailable);

private:
    void setupUi();
    void setupConnections();
    void setControlsEnabled(bool connected);
    void setCalibrationControlsEnabled(bool enabled);
    void setNetworkControlsEnabled(bool enabled);

    // Core objects
    SerialWorker *m_serialWorker;
    MotionDetector *m_motionDetector;
    Calibration *m_calibration;
    GraphWidget *m_graphWidget;
    bool m_calibrating;
    bool m_deviceStreaming = false;
    Calibration::Phase m_currentCalibrationPhase = Calibration::Phase::Idle;

    // Connection controls
    QComboBox *m_portCombo;
    QComboBox *m_baudCombo;
    QPushButton *m_refreshButton;
    QPushButton *m_connectButton;
    QLabel *m_connectionStatusLabel;

    // Network controls
    QPushButton *m_scanNetworksButton;
    QComboBox *m_networkCombo;
    QPushButton *m_connectNetworkButton;
    QPushButton *m_sniffNetworkButton;
    QLabel *m_networkStatusLabel;
    QVector<NetworkEntry> m_scannedNetworks;
    bool m_scanning = false;

    // Calibration controls
    QPushButton *m_calibrateButton;
    QDoubleSpinBox *m_warmupSpin;
    QDoubleSpinBox *m_scanSpin;
    QDoubleSpinBox *m_kSigmaSpin;
    QDoubleSpinBox *m_minThresholdSpin;
    QLabel *m_calibrationPhaseLabel;
    QProgressBar *m_calibrationProgressBar;

    // Live status
    QLabel *m_stateLabel;
    QLabel *m_wifiVarianceLabel;
    QLabel *m_bleVarianceLabel;
    QLabel *m_wifiThresholdLabel;
    QLabel *m_bleThresholdLabel;
    QProgressBar *m_debounceBar;
};

