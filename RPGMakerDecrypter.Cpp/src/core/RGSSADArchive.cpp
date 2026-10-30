#include "RGSSADArchive.h"

#include "ArchiveFileNameUtils.h"
#include "Constants.h"
#include "CoreExceptions.h"
#include "RGSSADV1Archive.h"
#include "RGSSADV3Archive.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include <cstring>

namespace RPGMakerDecrypter {

RGSSADArchive::RGSSADArchive(const QString& filePath)
{
    m_file = std::make_unique<QFile>(filePath);
    if (!m_file->open(QIODevice::ReadOnly)) {
        throw InvalidArchiveException(
            QStringLiteral("Unable to open archive '%1'. The file might be in use by another program.")
                .arg(filePath));
    }

    m_stream = std::make_unique<QDataStream>(m_file.get());
    m_stream->setByteOrder(QDataStream::LittleEndian);
}

RGSSADArchive::~RGSSADArchive() = default;

quint8 RGSSADArchive::readHeaderVersion(QDataStream& stream)
{
    // The header is a NUL terminated C string followed by a version byte.
    // Otherwise it is not a RGSSAD archive (or it is corrupted beyond use).
    QByteArray headerBytes(7, '\0');
    const qint64 bytesRead = stream.readRawData(headerBytes.data(), 7);
    if (bytesRead != 7) {
        throw InvalidArchiveException(QStringLiteral("Archive is in invalid format."));
    }

    const int nulIndex = headerBytes.indexOf('\0');
    if (nulIndex < 0 || headerBytes.left(nulIndex) != Constants::RGSSADHeader) {
        throw InvalidArchiveException(QStringLiteral("Header was not found for archive."));
    }

    const QByteArray versionBytes = stream.device()->peek(1);
    if (versionBytes.size() < 1) {
        throw InvalidArchiveException(QStringLiteral("Archive is in invalid format."));
    }
    const qint8 versionByte = versionBytes.at(0);

    // Seek to the end of the header.
    stream.device()->seek(8);

    return static_cast<quint8>(versionByte);
}

std::unique_ptr<RGSSADArchive> RGSSADArchive::open(const QString& filePath)
{
    // Peek at the header to figure out which format version the archive uses.
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        throw InvalidArchiveException(
            QStringLiteral("Unable to open archive '%1'. The file might be in use by another program.")
                .arg(filePath));
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);
    const quint8 version = readHeaderVersion(stream);
    file.close();

    if (version == Constants::RGSSADv1Version) {
        return std::make_unique<RGSSADV1Archive>(filePath);
    }
    if (version == Constants::RGSSADv3Version) {
        return std::make_unique<RGSSADV3Archive>(filePath);
    }

    throw InvalidArchiveException(QStringLiteral("Archive is not supported or it is corrupted."));
}

void RGSSADArchive::verifyVersionByte(quint8 expectedVersion)
{
    stream()->device()->seek(0);

    const quint8 version = readHeaderVersion(*stream());
    if (version != expectedVersion) {
        throw InvalidArchiveException(QStringLiteral("Archive is in invalid format."));
    }

    seekToHeaderEnd();
}

void RGSSADArchive::seekToHeaderEnd()
{
    stream()->device()->seek(8);
}

QByteArray RGSSADArchive::decryptFileData(const QByteArray& encryptedFileData, quint32 key)
{
    QByteArray decryptedFileData(encryptedFileData.size(), '\0');

    quint32 tempKey = key;
    quint8 keyBytes[4];
    std::memcpy(keyBytes, &tempKey, sizeof(keyBytes)); // little-endian on all supported platforms

    int j = 0;
    for (int i = 0; i < encryptedFileData.size(); ++i) {
        if (j == 4) {
            j = 0;
            tempKey *= 7;
            tempKey += 3;
            std::memcpy(keyBytes, &tempKey, sizeof(keyBytes));
        }

        decryptedFileData[i] = encryptedFileData[i] ^ keyBytes[j];
        ++j;
    }

    return decryptedFileData;
}

void RGSSADArchive::extractAllFiles(const QString& outputDirectoryPath,
                                    bool overwriteExisting,
                                    const ArchiveProgressCallback& progress,
                                    const ArchiveCancelCallback& cancelled)
{
    const int total = m_archivedFiles.size();

    int current = 0;
    for (const ArchiveEntry& archivedFile : m_archivedFiles) {
        if (cancelled && cancelled()) {
            throw OperationCancelledException();
        }

        extractFile(archivedFile, outputDirectoryPath, overwriteExisting);

        ++current;
        if (progress) {
            progress(current, total, archivedFile.name);
        }
    }
}

void RGSSADArchive::extractFile(const ArchiveEntry& archivedFile,
                                const QString& outputDirectoryPath,
                                bool overwriteExisting)
{
    const QString archiveFilePath = ArchiveFileNameUtils::getPlatformSpecificPath(archivedFile.name);

    // Paths in the archive are stored with the directory structure included.
    const qsizetype lastSeparatorIndex = archiveFilePath.lastIndexOf(QLatin1Char('/'));

    QString outputFilePath;
    if (lastSeparatorIndex >= 0) {
        const QString directoryPath = archiveFilePath.left(lastSeparatorIndex);
        if (!QDir(outputDirectoryPath).mkpath(directoryPath)) {
            throw DecrypterException(
                QStringLiteral("Unable to create directory '%1' under '%2'.")
                    .arg(directoryPath, outputDirectoryPath));
        }
        outputFilePath = outputDirectoryPath + QLatin1Char('/') + archiveFilePath;
    } else {
        outputFilePath = outputDirectoryPath + QLatin1Char('/') + archiveFilePath;
    }

    QFileInfo outputFileInfo(outputFilePath);
    if (outputFileInfo.exists() && !overwriteExisting) {
        return;
    }

    QFile outputFile(outputFilePath);
    if (!outputFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw DecrypterException(
            QStringLiteral("Unable to write file '%1'. The file might be open in another program or the output directory might not be writable.")
                .arg(outputFilePath));
    }

    QDataStream* stream = m_stream.get();
    stream->device()->seek(archivedFile.offset);

    QByteArray encryptedData(archivedFile.size, '\0');
    const qint64 bytesRead = stream->readRawData(encryptedData.data(), static_cast<int>(archivedFile.size));
    if (bytesRead != archivedFile.size) {
        throw InvalidArchiveException(QStringLiteral("Archive is invalid or corrupted. Reading failed."));
    }

    const QByteArray decryptedData = decryptFileData(encryptedData, archivedFile.key);
    outputFile.write(decryptedData);
}

RPGMakerVersion RGSSADArchive::getRPGMakerVersion(const QString& inputPath)
{
    const QFileInfo fileInfo(inputPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return RPGMakerVersion::Unknown;
    }

    const QString extension = fileInfo.suffix().toLower();

    if (extension == Constants::RpgMakerXpArchiveExtension) {
        return RPGMakerVersion::Xp;
    }

    if (extension == Constants::RpgMakerVxArchiveExtension) {
        return RPGMakerVersion::Vx;
    }

    if (extension == Constants::RpgMakerVxAceArchiveExtension) {
        return RPGMakerVersion::VxAce;
    }

    return RPGMakerVersion::Unknown;
}

} // namespace RPGMakerDecrypter
