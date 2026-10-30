#pragma once

#include "ArchiveEntry.h"
#include "RPGMakerVersion.h"

#include <QDataStream>
#include <QFile>
#include <QList>
#include <QString>

#include <functional>
#include <memory>

namespace RPGMakerDecrypter {

/// Callback invoked after each extracted file: (current, total, fileName).
using ArchiveProgressCallback = std::function<void(int current, int total, const QString& fileName)>;
/// Callback polled during long operations; returns true when the user wants to cancel.
using ArchiveCancelCallback = std::function<bool()>;

/// Represents RPG Maker RGSS Encrypted Archive (XP, VX and VX Ace).
class RGSSADArchive
{
public:
    virtual ~RGSSADArchive();

    RGSSADArchive(const RGSSADArchive&) = delete;
    RGSSADArchive& operator=(const RGSSADArchive&) = delete;

    /// Opens an archive and picks the correct format version automatically,
    /// based on the version byte in the archive header.
    /// Throws InvalidArchiveException for invalid or unsupported archives.
    static std::unique_ptr<RGSSADArchive> open(const QString& filePath);

    /// Validates the "RGSSAD" header and returns the version byte, or 0 if invalid.
    /// Both header reading functions seek to the end of the header before returning.
    static quint8 readHeaderVersion(QDataStream& stream);
    const QList<ArchiveEntry>& archivedFiles() const { return m_archivedFiles; }

    /// Extracts all files to the given output directory.
    void extractAllFiles(const QString& outputDirectoryPath,
                         bool overwriteExisting,
                         const ArchiveProgressCallback& progress = {},
                         const ArchiveCancelCallback& cancelled = {});

    /// Gets the RPG Maker version based on RGSSAD file extension.
    static RPGMakerVersion getRPGMakerVersion(const QString& inputPath);

protected:
    explicit RGSSADArchive(const QString& filePath);

    /// Overridden by concrete versions to read the file entries.
    virtual void readRGSSAD() = 0;

    /// Throws InvalidArchiveException if the header version byte doesn't match.
    void verifyVersionByte(quint8 expectedVersion);

    /// Seeks the stream to the first byte after the 8 byte header.
    void seekToHeaderEnd();

    QDataStream* stream() const { return m_stream.get(); }

    /// Decrypts file data using the given per-file key.
    static QByteArray decryptFileData(const QByteArray& encryptedFileData, quint32 key);

    QList<ArchiveEntry> m_archivedFiles;

private:
    void extractFile(const ArchiveEntry& archivedFile,
                     const QString& outputDirectoryPath,
                     bool overwriteExisting);

    std::unique_ptr<QFile> m_file;
    std::unique_ptr<QDataStream> m_stream;
};

} // namespace RPGMakerDecrypter
