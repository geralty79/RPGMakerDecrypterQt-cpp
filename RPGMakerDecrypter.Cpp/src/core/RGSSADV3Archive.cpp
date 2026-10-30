#include "RGSSADV3Archive.h"

#include "Constants.h"
#include "CoreExceptions.h"

#include <QDataStream>
#include <QIODevice>

#include <cstring>

namespace RPGMakerDecrypter {

RGSSADV3Archive::RGSSADV3Archive(const QString& filePath)
    : RGSSADArchive(filePath)
{
    // RGSSADv3 format is used by RPG Maker VX Ace.
    verifyVersionByte(Constants::RGSSADv3Version);
    readRGSSAD();
}

void RGSSADV3Archive::readRGSSAD()
{
    seekToHeaderEnd();

    QDataStream* stream = this->stream();

    // The archive key is stored right after the header.
    qint32 storedKey = 0;
    *stream >> storedKey;
    if (stream->status() != QDataStream::Ok) {
        throw InvalidArchiveException(QStringLiteral("Archive is invalid or corrupted. Reading failed."));
    }

    quint32 key = static_cast<quint32>(storedKey);
    key *= 9;
    key += 3;

    while (true) {
        ArchiveEntry archivedFile;

        qint32 encryptedOffset = 0;
        qint32 encryptedSize = 0;
        qint32 encryptedKey = 0;
        qint32 encryptedLength = 0;

        *stream >> encryptedOffset;
        *stream >> encryptedSize;
        *stream >> encryptedKey;
        *stream >> encryptedLength;
        if (stream->status() != QDataStream::Ok) {
            throw InvalidArchiveException(QStringLiteral("Archive is invalid or corrupted. Reading failed."));
        }

        // An offset of zero marks the end of the archive.
        archivedFile.offset = decryptInteger(encryptedOffset, key);
        if (archivedFile.offset == 0) {
            break;
        }

        archivedFile.size = decryptInteger(encryptedSize, key);
        archivedFile.key = static_cast<quint32>(decryptInteger(encryptedKey, key));

        if (archivedFile.size < 0) {
            throw InvalidArchiveException(QStringLiteral("Archive is invalid or corrupted. Reading failed."));
        }

        const qint32 nameLength = decryptInteger(encryptedLength, key);
        if (nameLength < 0) {
            throw InvalidArchiveException(QStringLiteral("Archive is invalid or corrupted. Reading failed."));
        }

        QByteArray encryptedName(nameLength, '\0');
        if (nameLength > 0) {
            const qint64 bytesRead = stream->readRawData(encryptedName.data(), nameLength);
            if (bytesRead != nameLength) {
                throw InvalidArchiveException(QStringLiteral("Archive is invalid or corrupted. Reading failed."));
            }
        }
        archivedFile.name = decryptFilename(encryptedName, key);

        m_archivedFiles.append(archivedFile);
    }
}

qint32 RGSSADV3Archive::decryptInteger(qint32 value, quint32 key)
{
    const qint32 result = static_cast<qint32>(static_cast<quint32>(value) ^ key);
    return result;
}

QString RGSSADV3Archive::decryptFilename(const QByteArray& encryptedName, quint32 key)
{
    QByteArray decryptedName(encryptedName.size(), '\0');

    quint8 keyBytes[4];
    std::memcpy(keyBytes, &key, sizeof(keyBytes)); // little-endian on all supported platforms

    int j = 0;
    for (int i = 0; i < encryptedName.size(); ++i) {
        if (j == 4) {
            j = 0;
        }

        decryptedName[i] = encryptedName[i] ^ static_cast<char>(keyBytes[j]);
        ++j;
    }

    return QString::fromUtf8(decryptedName);
}

} // namespace RPGMakerDecrypter
