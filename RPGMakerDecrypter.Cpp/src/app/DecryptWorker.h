#pragma once

#include "RPGMakerVersion.h"

#include <QObject>
#include <QString>

#include <atomic>

namespace RPGMakerDecrypter {

/// The complete user request for a single decrypt operation.
struct DecryptRequest
{
    QString inputPath;
    QString outputPath;
    bool reconstructProject = false;
    bool overwrite = false;
};

/// Runs decrypt operations on a background thread and reports progress.
class DecryptWorker : public QObject
{
    Q_OBJECT

public:
    explicit DecryptWorker(QObject* parent = nullptr);

public slots:
    void run(const DecryptRequest& request);

    /// Requests cancellation of the currently running operation.
    void cancel();

signals:
    /// Human readable progress message for the log.
    void message(const QString& text);

    /// Progress of the current operation: (current, total, currentItem).
    void progress(int current, int total, const QString& currentItem);

    /// Emitted when the operation has finished: (success, summary message).
    void finished(bool success, const QString& summary);

private:
    void runArchiveOperation(const DecryptRequest& request, RPGMakerVersion version);
    void runMVMZOperation(const DecryptRequest& request, RPGMakerVersion version);

    bool cancellationRequested();

    std::atomic<bool> m_cancelRequested;
};

} // namespace RPGMakerDecrypter
