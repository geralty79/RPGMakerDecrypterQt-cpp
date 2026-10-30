#include "EncryptionKeyFinder.h"

#include "CoreExceptions.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace RPGMakerDecrypter {

QByteArray EncryptionKeyFinder::findKey(const QString& inputPath)
{
    // System.json contains the encryption key.
    QDirIterator iterator(inputPath, { QStringLiteral("System.json") },
                          QDir::Files, QDirIterator::Subdirectories);

    QString systemFilePath;
    while (iterator.hasNext()) {
        systemFilePath = iterator.next();
        break;
    }

    if (systemFilePath.isEmpty()) {
        throw EncryptionKeyException(
            QStringLiteral("Unable to find the System.json file from the input path."));
    }

    QFile systemFile(systemFilePath);
    if (!systemFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        throw EncryptionKeyException(
            QStringLiteral("Unable to read the System.json file: '%1'.").arg(systemFilePath));
    }

    const QJsonDocument document = QJsonDocument::fromJson(systemFile.readAll());
    if (!document.isObject()) {
        throw EncryptionKeyException(
            QStringLiteral("System.json file could not be parsed: '%1'.").arg(systemFilePath));
    }

    const QJsonValue hashValue = document.object().value(QStringLiteral("encryptionKey"));
    if (!hashValue.isString()) {
        throw EncryptionKeyException(
            QStringLiteral("System.json file does not contain encryption key, unable to decrypt files. ")
            + QStringLiteral("It's also possible that the files are not encrypted."));
    }

    // The encryption key is a MD5 hash of the key given when deploying the game.
    const QString md5Hash = hashValue.toString();

    if (md5Hash.isEmpty() || md5Hash.trimmed().isEmpty()) {
        throw EncryptionKeyException(
            QStringLiteral("System.json file does not contain encryption key, unable to decrypt files. ")
            + QStringLiteral("It's also possible that the files are not encrypted."));
    }

    if (md5Hash.length() != 32) {
        throw EncryptionKeyException(QStringLiteral("Found encryption key but it is too short."));
    }

    // The MD5 hash converted into 16 bytes is the actual encryption key.
    const QByteArray key = QByteArray::fromHex(md5Hash.toLatin1());
    if (key.size() != 16) {
        throw EncryptionKeyException(QStringLiteral("Found encryption key but it is invalid."));
    }

    return key;
}

} // namespace RPGMakerDecrypter
