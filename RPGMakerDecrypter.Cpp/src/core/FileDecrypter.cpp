#include "FileDecrypter.h"

#include <QFile>
#include <QIODevice>

namespace RPGMakerDecrypter {

QByteArray FileDecrypter::decrypt(const QByteArray& key, const QString& inputPath)
{
    QFile file(inputPath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QByteArray encryptedData = file.readAll();

    if (encryptedData.size() <= 16) {
        // Not enough data for the fake header, nothing to decrypt.
        return {};
    }

    // Skip the first 16 bytes of the file (the fake header).
    QByteArray decryptedData = encryptedData.mid(16);

    // XOR back the first 16 bytes of the file data, which are encrypted with the key,
    // to get the real file data.
    const int xorLength = qMin(16, decryptedData.size());
    for (int i = 0; i < xorLength; ++i) {
        decryptedData[i] = decryptedData[i] ^ key[i];
    }

    return decryptedData;
}

} // namespace RPGMakerDecrypter
