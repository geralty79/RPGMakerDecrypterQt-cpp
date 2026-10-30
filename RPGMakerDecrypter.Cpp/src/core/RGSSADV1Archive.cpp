#include "RGSSADV1Archive.h"

#include "Constants.h"
#include "CoreExceptions.h"

#include <QDataStream>
#include <QIODevice>

namespace RPGMakerDecrypter {

RGSSADV1Archive::RGSSADV1Archive(const QString& filePath)
    : RGSSADArchive(filePath)
{
    // RGSSADv1 format is used by RPG Maker XP and VX.
    verifyVersionByte(Constants::RGSSADv1Version);
    readRGSSAD();
}

void RGSSADV1Archive::readRGSSAD()
{
    quint32 key = Constants::RGSSADv1Key;

    seekToHeaderEnd();

    QDataStream* stream = this->stream();
    QFile* device = qobject_cast<QFile*>(stream->device());
    const qint64 archiveSize = device->size();

    while (true) {
        ArchiveEntry archivedFile;

        qint32 encryptedLength = 0;
        *stream >> encryptedLength;
        if (stream->status() != QDataStream::Ok) {
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

        qint32 encryptedSize = 0;
        *stream >> encryptedSize;
        if (stream->status() != QDataStream::Ok) {
            throw InvalidArchiveException(QStringLiteral("Archive is invalid or corrupted. Reading failed."));
        }
        archivedFile.size = decryptInteger(encryptedSize, key);

        archivedFile.offset = stream->device()->pos();
        archivedFile.key = key;
        m_archivedFiles.append(archivedFile);

        // Skip over the file data.
        stream->device()->seek(stream->device()->pos() + archivedFile.size);

        if (stream->device()->pos() >= archiveSize) {
            break;
        }
    }
}

qint32 RGSSADV1Archive::decryptInteger(qint32 value, quint32& key)
{
    const qint32 result = static_cast<qint32>(static_cast<quint32>(value) ^ key);

    key *= 7;
    key += 3;

    return result;
}

QString RGSSADV1Archive::decryptFilename(const QByteArray& encryptedName, quint32& key)
{
    QByteArray decryptedName(encryptedName.size(), '\0');

    for (int i = 0; i < encryptedName.size(); ++i) {
        decryptedName[i] = encryptedName[i] ^ static_cast<char>(key & 0xFF);

        key *= 7;
        key += 3;
    }

    return QString::fromUtf8(decryptedName);
}

} // namespace RPGMakerDecrypter
