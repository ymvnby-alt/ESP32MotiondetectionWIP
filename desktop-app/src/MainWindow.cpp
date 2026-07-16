#include "MainWindow.h"

#include <QSerialPortInfo>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QStatusBar>
#include <QMessageBox>
#include <QInputDialog>
#include <QLineEdit>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_serialWorker(new SerialWorker(this)),
      m_motionDetector(new MotionDetector(this)),
      m_calibration(new Calibration(this)),
      m_graphWidget(nullptr),
      m_calibrating(false) {
    setupUi();
    setupConnections();
    onRefreshPortsClicked();
    setControlsEnabled(false);
    setCalibrationControlsEnabled(false);
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUi() {
    setWindowTitle("CSI Motion Detector");
    resize(1000, 700);

    auto *central = new QWidget(this);
    auto *rootLayout = new QVBoxLayout(central);

    // --- Connection group ---
    auto *connGroup = new QGroupBox("Serial Connection", central);
    auto *connLayout = new QHBoxLayout(connGroup);

    m_portCombo = new QComboBox(connGroup);
    m_baudCombo = new QComboBox(connGroup);
    m_baudCombo->addItems({"9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"});
    m_baudCombo->setCurrentText("921600");

    m_refreshButton = new QPushButton("Refresh", connGroup);
    m_connectButton = new QPushButton("Connect", connGroup);
    m_connectionStatusLabel = new QLabel("Disconnected", connGroup);

    connLayout->addWidget(new QLabel("Port:", connGroup));
    connLayout->addWidget(m_portCombo);
    connLayout->addWidget(new QLabel("Baud:", connGroup));
    connLayout->addWidget(m_baudCombo);
    connLayout->addWidget(m_refreshButton);
    connLayout->addWidget(m_connectButton);
    connLayout->addStretch();
    connLayout->addWidget(m_connectionStatusLabel);

    // --- Network group ---
    auto *netGroup = new QGroupBox("WiFi Network", central);
    auto *netLayout = new QHBoxLayout(netGroup);

    m_scanNetworksButton = new QPushButton("Scan Networks", netGroup);
    m_networkCombo = new QComboBox(netGroup);
    m_networkCombo->setMinimumWidth(250);
    m_connectNetworkButton = new QPushButton("Connect (password)", netGroup);
    m_sniffNetworkButton = new QPushButton("Sniff (no password)", netGroup);
    m_networkStatusLabel = new QLabel("Not connected to a network", netGroup);

    netLayout->addWidget(m_scanNetworksButton);
    netLayout->addWidget(m_networkCombo);
    netLayout->addWidget(m_connectNetworkButton);
    netLayout->addWidget(m_sniffNetworkButton);
    netLayout->addStretch();
    netLayout->addWidget(m_networkStatusLabel);

    // --- Calibration group ---
    auto *calGroup = new QGroupBox("Calibration", central);
    auto *calLayout = new QVBoxLayout(calGroup);
    auto *calFormLayout = new QFormLayout();

    m_warmupSpin = new QDoubleSpinBox(calGroup);
    m_warmupSpin->setRange(1.0, 120.0);
    m_warmupSpin->setValue(10.0);
    m_warmupSpin->setSuffix(" s");

    m_scanSpin = new QDoubleSpinBox(calGroup);
    m_scanSpin->setRange(5.0, 300.0);
    m_scanSpin->setValue(30.0);
    m_scanSpin->setSuffix(" s");

    m_kSigmaSpin = new QDoubleSpinBox(calGroup);
    m_kSigmaSpin->setRange(1.0, 20.0);
    m_kSigmaSpin->setValue(5.0);
    m_kSigmaSpin->setSingleStep(0.5);

    m_minThresholdSpin = new QDoubleSpinBox(calGroup);
    m_minThresholdSpin->setRange(0.0, 1000.0);
    m_minThresholdSpin->setValue(10.0);

    calFormLayout->addRow("Warmup time:", m_warmupSpin);
    calFormLayout->addRow("Scan time:", m_scanSpin);
    calFormLayout->addRow("K-sigma:", m_kSigmaSpin);
    calFormLayout->addRow("Min threshold:", m_minThresholdSpin);

    m_calibrateButton = new QPushButton("Start Calibration", calGroup);
    m_calibrationPhaseLabel = new QLabel("Not calibrated", calGroup);
    m_calibrationProgressBar = new QProgressBar(calGroup);
    m_calibrationProgressBar->setRange(0, 100);
    m_calibrationProgressBar->setValue(0);

    calLayout->addLayout(calFormLayout);
    calLayout->addWidget(m_calibrateButton);
    calLayout->addWidget(m_calibrationPhaseLabel);
    calLayout->addWidget(m_calibrationProgressBar);

    // --- Status group ---
    auto *statusGroup = new QGroupBox("Live Status", central);
    auto *statusForm = new QFormLayout(statusGroup);

    m_stateLabel = new QLabel("idle", statusGroup);
    QFont stateFont = m_stateLabel->font();
    stateFont.setBold(true);
    stateFont.setPointSize(stateFont.pointSize() + 2);
    m_stateLabel->setFont(stateFont);

    m_wifiVarianceLabel = new QLabel("-", statusGroup);
    m_bleVarianceLabel = new QLabel("-", statusGroup);
    m_wifiThresholdLabel = new QLabel("-", statusGroup);
    m_bleThresholdLabel = new QLabel("-", statusGroup);
    m_debounceBar = new QProgressBar(statusGroup);
    m_debounceBar->setRange(0, 1);
    m_debounceBar->setValue(0);

    statusForm->addRow("State:", m_stateLabel);
    statusForm->addRow("WiFi variance:", m_wifiVarianceLabel);
    statusForm->addRow("WiFi threshold:", m_wifiThresholdLabel);
    statusForm->addRow("BLE variance:", m_bleVarianceLabel);
    statusForm->addRow("BLE threshold:", m_bleThresholdLabel);
    statusForm->addRow("Debounce:", m_debounceBar);

    // --- Top row: calibration + status side by side ---
    auto *topRow = new QHBoxLayout();
    topRow->addWidget(calGroup, 1);
    topRow->addWidget(statusGroup, 1);

    // --- Graph ---
    m_graphWidget = new GraphWidget(central);

    rootLayout->addWidget(connGroup);
    rootLayout->addWidget(netGroup);
    rootLayout->addLayout(topRow);
    rootLayout->addWidget(m_graphWidget, 1);

    setCentralWidget(central);
    statusBar()->showMessage("Ready");
}

void MainWindow::setupConnections() {
    connect(m_refreshButton, &QPushButton::clicked, this, &MainWindow::onRefreshPortsClicked);
    connect(m_connectButton, &QPushButton::clicked, this, &MainWindow::onConnectClicked);
    connect(m_calibrateButton, &QPushButton::clicked, this, &MainWindow::onCalibrateClicked);
    connect(m_scanNetworksButton, &QPushButton::clicked, this, &MainWindow::onScanNetworksClicked);
    connect(m_connectNetworkButton, &QPushButton::clicked, this, &MainWindow::onConnectNetworkClicked);
    connect(m_sniffNetworkButton, &QPushButton::clicked, this, &MainWindow::onSniffNetworkClicked);

    connect(m_serialWorker, &SerialWorker::portOpened, this, &MainWindow::onPortOpened);
    connect(m_serialWorker, &SerialWorker::portClosed, this, &MainWindow::onPortClosed);
    connect(m_serialWorker, &SerialWorker::errorOccurred, this, &MainWindow::onSerialError);
    connect(m_serialWorker, &SerialWorker::csiReceived, this, &MainWindow::onCsiReceived);
    connect(m_serialWorker, &SerialWorker::bleReceived, this, &MainWindow::onBleReceived);
    connect(m_serialWorker, &SerialWorker::textLineReceived, this, &MainWindow::onTextLineReceived);

    connect(m_motionDetector, &MotionDetector::wifiVarianceUpdated, this, &MainWindow::onWifiVarianceUpdated);
    connect(m_motionDetector, &MotionDetector::bleVarianceUpdated, this, &MainWindow::onBleVarianceUpdated);
    connect(m_motionDetector, &MotionDetector::stateChanged, this, &MainWindow::onMotionStateChanged);
    connect(m_motionDetector, &MotionDetector::debounceProgress, this, &MainWindow::onDebounceProgress);

    connect(m_calibration, &Calibration::phaseChanged, this, &MainWindow::onCalibrationPhaseChanged);
    connect(m_calibration, &Calibration::progressUpdated, this, &MainWindow::onCalibrationProgress);
    connect(m_calibration, &Calibration::finished, this, &MainWindow::onCalibrationFinished);
}

void MainWindow::setControlsEnabled(bool connected) {
    m_portCombo->setEnabled(!connected);
    m_baudCombo->setEnabled(!connected);
    m_refreshButton->setEnabled(!connected);
    m_connectButton->setText(connected ? "Disconnect" : "Connect");
    setNetworkControlsEnabled(connected);
    if (!connected) {
        setCalibrationControlsEnabled(false);
    }
}

void MainWindow::setNetworkControlsEnabled(bool enabled) {
    m_scanNetworksButton->setEnabled(enabled && !m_scanning);
    bool haveNetworks = !m_scannedNetworks.isEmpty();
    m_networkCombo->setEnabled(enabled && haveNetworks && !m_scanning);
    m_connectNetworkButton->setEnabled(enabled && haveNetworks && !m_scanning);
    m_sniffNetworkButton->setEnabled(enabled && haveNetworks && !m_scanning);
}

void MainWindow::setCalibrationControlsEnabled(bool enabled) {
    m_calibrateButton->setEnabled(enabled && !m_calibrating);
    m_warmupSpin->setEnabled(enabled && !m_calibrating);
    m_scanSpin->setEnabled(enabled && !m_calibrating);
    m_kSigmaSpin->setEnabled(enabled && !m_calibrating);
    m_minThresholdSpin->setEnabled(enabled && !m_calibrating);
}

void MainWindow::onRefreshPortsClicked() {
    QString previous = m_portCombo->currentText();
    m_portCombo->clear();
    const auto ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo &info : ports) {
        m_portCombo->addItem(info.portName());
    }
    int idx = m_portCombo->findText(previous);
    if (idx >= 0) {
        m_portCombo->setCurrentIndex(idx);
    }
}

void MainWindow::onConnectClicked() {
    if (m_connectButton->text() == "Connect") {
        if (m_portCombo->currentText().isEmpty()) {
            QMessageBox::warning(this, "No port selected", "Please select a serial port first.");
            return;
        }
        qint32 baud = m_baudCombo->currentText().toInt();
        m_serialWorker->openPort(m_portCombo->currentText(), baud);
    } else {
        m_serialWorker->closePort();
    }
}

void MainWindow::onPortOpened() {
    m_connectionStatusLabel->setText("Connected: " + m_portCombo->currentText());
    setControlsEnabled(true);
    m_motionDetector->reset();
    m_graphWidget->clearData();
    m_deviceStreaming = false;
    m_scannedNetworks.clear();
    m_networkCombo->clear();
    m_networkStatusLabel->setText("Not connected to a network");
    statusBar()->showMessage("Serial port opened", 3000);
    onScanNetworksClicked();
}

void MainWindow::onPortClosed() {
    m_connectionStatusLabel->setText("Disconnected");
    setControlsEnabled(false);
    m_calibrating = false;
    m_deviceStreaming = false;
    m_scanning = false;
    m_scannedNetworks.clear();
    m_networkCombo->clear();
    m_networkStatusLabel->setText("Not connected to a network");
    statusBar()->showMessage("Serial port closed", 3000);
}

void MainWindow::onSerialError(const QString &message) {
    statusBar()->showMessage("Serial error: " + message, 5000);
    QMessageBox::warning(this, "Serial Error", message);
}

void MainWindow::onCalibrateClicked() {
    if (m_calibrating) {
        return;
    }
    m_calibrating = true;
    setCalibrationControlsEnabled(true);
    m_motionDetector->reset();
    m_graphWidget->clearData();
    m_calibration->start(
        10,
        m_warmupSpin->value(),
        m_scanSpin->value(),
        m_kSigmaSpin->value(),
        m_minThresholdSpin->value()
    );
}

void MainWindow::onCsiReceived(CsiPacket packet) {
    if (m_calibrating) {
        m_calibration->processCsi(packet);
    } else {
        m_motionDetector->processCsi(packet);
    }
}

void MainWindow::onBleReceived(BlePacket packet) {
    if (m_calibrating) {
        m_calibration->processBle(packet);
    } else {
        m_motionDetector->processBle(packet);
    }
}

void MainWindow::onScanNetworksClicked() {
    if (m_connectButton->text() != "Disconnect") {
        return;
    }
    m_scanning = true;
    m_scannedNetworks.clear();
    m_networkCombo->clear();
    m_networkStatusLabel->setText("Scanning...");
    setNetworkControlsEnabled(true);
    m_serialWorker->sendCommand("SCAN");
}

void MainWindow::onConnectNetworkClicked() {
    int idx = m_networkCombo->currentIndex();
    if (idx < 0 || idx >= m_scannedNetworks.size()) {
        return;
    }
    const NetworkEntry &entry = m_scannedNetworks[idx];

    bool ok = false;
    QString password = QInputDialog::getText(
        this, "Network Password",
        QString("Enter password for '%1' (leave blank if open):").arg(entry.ssid),
        QLineEdit::Password, "", &ok
    );
    if (!ok) {
        return;
    }

    m_networkStatusLabel->setText("Connecting to " + entry.ssid + "...");
    setNetworkControlsEnabled(false);
    m_serialWorker->sendCommand(QString("CONNECT|%1|%2").arg(entry.ssid, password));
}

void MainWindow::onSniffNetworkClicked() {
    int idx = m_networkCombo->currentIndex();
    if (idx < 0 || idx >= m_scannedNetworks.size()) {
        return;
    }
    const NetworkEntry &entry = m_scannedNetworks[idx];

    m_networkStatusLabel->setText(QString("Sniffing channel %1 (no password)...").arg(entry.channel));
    setNetworkControlsEnabled(false);
    m_serialWorker->sendCommand(QString("SNIFF|%1").arg(entry.channel));
}

void MainWindow::onTextLineReceived(const QString &line) {
    if (line.startsWith("NET|")) {
        QStringList parts = line.split('|');
        if (parts.size() == 5) {
            NetworkEntry entry;
            entry.index = parts[1].toInt();
            entry.ssid = parts[2];
            entry.channel = parts[3].toInt();
            entry.rssi = parts[4].toInt();
            m_scannedNetworks.append(entry);
            m_networkCombo->addItem(QString("%1  (ch %2, %3 dBm)").arg(entry.ssid).arg(entry.channel).arg(entry.rssi));
        }
        return;
    }

    if (line == "SCANDONE") {
        m_scanning = false;
        setNetworkControlsEnabled(true);
        m_networkStatusLabel->setText(
            m_scannedNetworks.isEmpty()
                ? "No networks found - try Scan again"
                : QString("%1 networks found - select one, then Connect or Sniff").arg(m_scannedNetworks.size())
        );
        return;
    }

    if (line.startsWith("READY,")) {
        m_deviceStreaming = true;
        setNetworkControlsEnabled(true);
        setCalibrationControlsEnabled(true);
        m_networkStatusLabel->setText("Streaming: " + line.section(',', 1));
        statusBar()->showMessage("Device is now streaming CSI/BLE data", 3000);
        return;
    }

    if (line.startsWith("CONNECT_FAILED") || line.startsWith("SNIFF_FAILED")) {
        setNetworkControlsEnabled(true);
        m_networkStatusLabel->setText("Failed: " + line + " - try again");
        QMessageBox::warning(this, "Connection Failed", line);
        return;
    }

    statusBar()->showMessage("Device: " + line, 2000);
}

void MainWindow::onWifiVarianceUpdated(double variance, int rssi) {
    m_wifiVarianceLabel->setText(QString::number(variance, 'f', 2) + QString(" (RSSI %1)").arg(rssi));
    m_graphWidget->addWifiSample(variance);
}

void MainWindow::onBleVarianceUpdated(double variance) {
    m_bleVarianceLabel->setText(QString::number(variance, 'f', 2));
    m_graphWidget->addBleSample(variance);
}

void MainWindow::onMotionStateChanged(const QString &state) {
    m_stateLabel->setText(state);
    if (state == "motion") {
        m_stateLabel->setStyleSheet("color: #c0392b;");
    } else if (state == "unconfirmed") {
        m_stateLabel->setStyleSheet("color: #d68910;");
    } else {
        m_stateLabel->setStyleSheet("color: #27ae60;");
    }
    m_serialWorker->sendMotionState(state == "motion");
}

void MainWindow::onDebounceProgress(int current, int total) {
    m_debounceBar->setRange(0, total);
    m_debounceBar->setValue(current);
}

void MainWindow::onCalibrationPhaseChanged(Calibration::Phase phase, QString message) {
    m_calibrationPhaseLabel->setText(message);
    m_currentCalibrationPhase = phase;

    if (phase == Calibration::Phase::Done) {
        m_calibrating = false;
        setCalibrationControlsEnabled(true);
        m_calibrationProgressBar->setValue(0);
    }
}

void MainWindow::onCalibrationProgress(double remainingSeconds, int samplesCollected) {
    Q_UNUSED(samplesCollected);

    double warmup = m_warmupSpin->value();
    double scan = m_scanSpin->value();
    double total = 2.0 * (warmup + scan);
    if (total <= 0) {
        m_calibrationProgressBar->setValue(0);
        return;
    }

    double cumulativeBefore = 0.0;
    double phaseDuration = 0.0;

    switch (m_currentCalibrationPhase) {
        case Calibration::Phase::WarmupEmpty:
            cumulativeBefore = 0.0;
            phaseDuration = warmup;
            break;
        case Calibration::Phase::ScanningEmpty:
            cumulativeBefore = warmup;
            phaseDuration = scan;
            break;
        case Calibration::Phase::WarmupMotion:
            cumulativeBefore = warmup + scan;
            phaseDuration = warmup;
            break;
        case Calibration::Phase::ScanningMotion:
            cumulativeBefore = 2.0 * warmup + scan;
            phaseDuration = scan;
            break;
        default:
            m_calibrationProgressBar->setValue(0);
            return;
    }

    double elapsedInPhase = phaseDuration - remainingSeconds;
    double totalElapsed = cumulativeBefore + elapsedInPhase;
    int pct = static_cast<int>(100.0 * totalElapsed / total);
    m_calibrationProgressBar->setValue(qBound(0, pct, 100));
}

void MainWindow::onCalibrationFinished(double wifiThreshold, double bleThreshold, bool bleAvailable) {
    m_motionDetector->setWifiThreshold(wifiThreshold);
    m_motionDetector->setBleThreshold(bleThreshold, bleAvailable);
    m_motionDetector->reset();

    m_wifiThresholdLabel->setText(QString::number(wifiThreshold, 'f', 2));
    m_bleThresholdLabel->setText(bleAvailable ? QString::number(bleThreshold, 'f', 2) : "N/A");

    m_graphWidget->setWifiThreshold(wifiThreshold);
    m_graphWidget->setBleThreshold(bleThreshold, bleAvailable);
    m_graphWidget->clearData();

    statusBar()->showMessage("Calibration finished", 4000);
}
