#include "MainWindow.h"

#include "DecryptWorker.h"
#include "VersionFinder.h"

#include <QCheckBox>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QStatusBar>
#include <QStyle>
#include <QThread>
#include <QTime>
#include <QVBoxLayout>

namespace RPGMakerDecrypter {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    createUi();
}

MainWindow::~MainWindow()
{
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }
}

void MainWindow::createUi()
{
    setWindowTitle(tr("RPG Maker Decrypter"));
    resize(680, 520);

    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setSpacing(12);

    // Input path
    auto* inputFormLayout = new QFormLayout();
    inputFormLayout->setLabelAlignment(Qt::AlignRight);

    m_inputPathEdit = new QLineEdit(centralWidget);
    m_inputPathEdit->setPlaceholderText(
        tr("Path to a .rgssad, .rgss2a or .rgss3a file, or to an MV or MZ deployment directory"));
    m_inputPathEdit->setClearButtonEnabled(true);

    m_versionLabel = new QLabel(tr("No input selected"), centralWidget);

    auto* inputButtonLayout = new QHBoxLayout();
    auto* browseFileButton = new QPushButton(tr("Browse File..."), centralWidget);
    auto* browseDirectoryButton = new QPushButton(tr("Browse Folder..."), centralWidget);
    inputButtonLayout->addWidget(browseFileButton);
    inputButtonLayout->addWidget(browseDirectoryButton);
    inputButtonLayout->addStretch(1);

    inputFormLayout->addRow(tr("Input:"), m_inputPathEdit);
    inputFormLayout->addRow(QString(), inputButtonLayout);
    inputFormLayout->addRow(tr("Detected version:"), m_versionLabel);

    mainLayout->addLayout(inputFormLayout);

    // Output path
    auto* outputFormLayout = new QFormLayout();
    outputFormLayout->setLabelAlignment(Qt::AlignRight);

    m_outputPathEdit = new QLineEdit(centralWidget);
    m_outputPathEdit->setPlaceholderText(
        tr("Optional. By default files are extracted next to the input"));
    m_outputPathEdit->setClearButtonEnabled(true);

    auto* outputButtonLayout = new QHBoxLayout();
    auto* browseOutputButton = new QPushButton(tr("Browse..."), centralWidget);
    auto* clearOutputButton = new QPushButton(tr("Clear"), centralWidget);
    outputButtonLayout->addWidget(browseOutputButton);
    outputButtonLayout->addWidget(clearOutputButton);
    outputButtonLayout->addStretch(1);

    outputFormLayout->addRow(tr("Output:"), m_outputPathEdit);
    outputFormLayout->addRow(QString(), outputButtonLayout);

    mainLayout->addLayout(outputFormLayout);

    // Options
    m_reconstructProjectCheckBox =
        new QCheckBox(tr("Reconstruct project (creates the project files for the editor)"), centralWidget);
    m_overwriteCheckBox =
        new QCheckBox(tr("Overwrite existing files and directories"), centralWidget);

    auto* optionsLayout = new QVBoxLayout();
    optionsLayout->addWidget(m_reconstructProjectCheckBox);
    optionsLayout->addWidget(m_overwriteCheckBox);
    mainLayout->addLayout(optionsLayout);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    m_startButton = new QPushButton(style()->standardIcon(QStyle::SP_MediaPlay), tr("Start"), centralWidget);
    m_cancelButton = new QPushButton(tr("Cancel"), centralWidget);
    m_cancelButton->setEnabled(false);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_startButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);

    // Progress
    m_progressBar = new QProgressBar(centralWidget);
    m_progressBar->setVisible(false);
    mainLayout->addWidget(m_progressBar);

    // Log
    m_log = new QPlainTextEdit(centralWidget);
    m_log->setReadOnly(true);
    mainLayout->addWidget(m_log, 1);

    setCentralWidget(centralWidget);

    connect(m_inputPathEdit, &QLineEdit::textChanged, this, &MainWindow::updateVersionLabel);

    connect(browseFileButton, &QPushButton::clicked, this, &MainWindow::browseInputFile);
    connect(browseDirectoryButton, &QPushButton::clicked, this, &MainWindow::browseInputDirectory);
    connect(browseOutputButton, &QPushButton::clicked, this, &MainWindow::browseOutputDirectory);
    connect(clearOutputButton, &QPushButton::clicked, this, &MainWindow::clearOutputDirectory);

    connect(m_startButton, &QPushButton::clicked, this, &MainWindow::start);
    connect(m_cancelButton, &QPushButton::clicked, this, &MainWindow::cancel);
}

void MainWindow::browseInputFile()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this,
        tr("Select RPG Maker archive file"),
        m_inputPathEdit->text(),
        tr("RPG Maker archives (*.rgssad *.rgss2a *.rgss3a)"));

    if (filePath.isEmpty()) {
        return;
    }

    m_inputPathEdit->setText(filePath);
}

void MainWindow::browseInputDirectory()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(
        this,
        tr("Select RPG Maker MV or MZ deployment directory"),
        m_inputPathEdit->text());

    if (directoryPath.isEmpty()) {
        return;
    }

    m_inputPathEdit->setText(directoryPath);
}

void MainWindow::browseOutputDirectory()
{
    const QString directoryPath = QFileDialog::getExistingDirectory(
        this,
        tr("Select output directory"),
        m_outputPathEdit->text());

    if (directoryPath.isEmpty()) {
        return;
    }

    m_outputPathEdit->setText(directoryPath);
}

void MainWindow::clearOutputDirectory()
{
    m_outputPathEdit->clear();
}

void MainWindow::updateVersionLabel()
{
    const QString inputPath = m_inputPathEdit->text();
    const QFileInfo inputInfo(inputPath);

    if (inputPath.isEmpty()) {
        m_versionLabel->setText(tr("No input selected"));
        return;
    }

    if (!inputInfo.exists()) {
        m_versionLabel->setText(tr("Path does not exist"));
        return;
    }

    const RPGMakerVersion version = VersionFinder::findVersion(inputPath);
    if (version == RPGMakerVersion::Unknown) {
        m_versionLabel->setText(tr("Unable to determine the RPG Maker version"));
        return;
    }

    m_versionLabel->setText(toDisplayString(version));
}

DecryptRequest MainWindow::currentRequest() const
{
    DecryptRequest request;
    request.inputPath = m_inputPathEdit->text().trimmed();
    request.outputPath = m_outputPathEdit->text().trimmed();
    request.reconstructProject = m_reconstructProjectCheckBox->isChecked();
    request.overwrite = m_overwriteCheckBox->isChecked();
    return request;
}

void MainWindow::start()
{
    const DecryptRequest request = currentRequest();
    const QFileInfo inputInfo(request.inputPath);

    if (request.inputPath.isEmpty() || !inputInfo.exists()) {
        QMessageBox::warning(this, tr("RPG Maker Decrypter"),
                             tr("Please select a valid input file or directory."));
        return;
    }

    const RPGMakerVersion version = VersionFinder::findVersion(request.inputPath);
    if (version == RPGMakerVersion::Unknown) {
        QMessageBox::warning(
            this, tr("RPG Maker Decrypter"),
            tr("Unable to determine the RPG Maker version. ")
                + tr("Please rename the RGSSAD file with an extension corresponding to the version: ")
                + QStringLiteral("XP: .rgssad, VX: .rgss2a, VX Ace: .rgss3a ")
                + tr("or point to an MV or MZ directory."));
        return;
    }

    if (request.reconstructProject
        && (version == RPGMakerVersion::Mv || version == RPGMakerVersion::Mz)
        && request.outputPath.trimmed().isEmpty()) {
        QMessageBox::warning(
            this, tr("RPG Maker Decrypter"),
            tr("MV and MZ project reconstruction requires an output path."));
        return;
    }

    if (m_workerThread) {
        // The worker thread from a previous operation has already finished,
        // clean it up before starting a new one.
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
        m_workerThread = nullptr;
        m_worker = nullptr;
    }

    m_workerThread = new QThread(this);
    m_worker = new DecryptWorker();
    m_worker->moveToThread(m_workerThread);

    connect(m_workerThread, &QThread::finished, m_worker, &QObject::deleteLater);
    connect(this, &MainWindow::startWorkRequested, m_worker, &DecryptWorker::run);
    connect(m_worker, &DecryptWorker::message, this, &MainWindow::appendLog, Qt::QueuedConnection);
    connect(m_worker, &DecryptWorker::progress, this, &MainWindow::updateProgress, Qt::QueuedConnection);
    connect(m_worker, &DecryptWorker::finished, this, &MainWindow::onWorkFinished, Qt::QueuedConnection);

    m_log->clear();
    appendLog(tr("Operation started."));

    setWorking(true);
    m_workerThread->start();

    emit startWorkRequested(request);
}

void MainWindow::cancel()
{
    if (m_worker) {
        m_cancelButton->setEnabled(false);
        m_worker->cancel();
    }
}

void MainWindow::onWorkFinished(bool success, const QString& summary)
{
    setWorking(false);
    m_worker = nullptr;

    appendLog(summary);

    if (!success) {
        QMessageBox::critical(this, tr("RPG Maker Decrypter"), summary);
    } else {
        statusBar()->showMessage(summary, 5000);
    }

    if (m_workerThread) {
        m_workerThread->quit();
    }

    updateVersionLabel();
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    if (m_worker) {
        m_worker->cancel();
    }

    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
    }

    QMainWindow::closeEvent(event);
}

void MainWindow::setWorking(bool working)
{
    m_startButton->setEnabled(!working);
    m_cancelButton->setEnabled(working);

    m_inputPathEdit->setEnabled(!working);
    m_outputPathEdit->setEnabled(!working);
    m_reconstructProjectCheckBox->setEnabled(!working);
    m_overwriteCheckBox->setEnabled(!working);

    m_progressBar->setVisible(working);
    if (!working) {
        m_progressBar->reset();
    }
}

void MainWindow::appendLog(const QString& text)
{
    m_log->appendPlainText(QTime::currentTime().toString(QStringLiteral("hh:mm:ss ")) + text);
}

void MainWindow::updateProgress(int current, int total, const QString& currentItem)
{
    if (total <= 0) {
        m_progressBar->setRange(0, 0); // busy indicator
        return;
    }

    m_progressBar->setRange(0, total);
    m_progressBar->setValue(current);
    statusBar()->showMessage(currentItem, 2000);
}

} // namespace RPGMakerDecrypter
