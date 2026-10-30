#pragma once

#include "DecryptWorker.h"
#include "RPGMakerVersion.h"

#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QThread>

namespace RPGMakerDecrypter {

/// Main window of the RPG Maker Decrypter GUI.
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void browseInputFile();
    void browseInputDirectory();
    void browseOutputDirectory();
    void clearOutputDirectory();
    void updateVersionLabel();
    void start();
    void cancel();
    void onWorkFinished(bool success, const QString& summary);

private:
    void createUi();
    void setWorking(bool working);
    void appendLog(const QString& text);
    void updateProgress(int current, int total, const QString& currentItem);
    DecryptRequest currentRequest() const;

signals:
    void startWorkRequested(const DecryptRequest& request);

private:
    QLabel* m_versionLabel = nullptr;
    QLineEdit* m_inputPathEdit = nullptr;
    QLineEdit* m_outputPathEdit = nullptr;

    QCheckBox* m_reconstructProjectCheckBox = nullptr;
    QCheckBox* m_overwriteCheckBox = nullptr;

    QPushButton* m_startButton = nullptr;
    QPushButton* m_cancelButton = nullptr;

    QProgressBar* m_progressBar = nullptr;
    QPlainTextEdit* m_log = nullptr;

    QThread* m_workerThread = nullptr;
    DecryptWorker* m_worker = nullptr;
};

} // namespace RPGMakerDecrypter
