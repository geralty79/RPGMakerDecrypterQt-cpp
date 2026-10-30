#pragma once

#include <QByteArray>
#include <QString>

namespace RPGMakerDecrypter {

/// Decrypts a single encrypted MV/MZ file.
class FileDecrypter
{
public:
    /// Decrypts an encrypted RPG Maker file using the provided key and input path.
    /// Returns the decrypted file data.
    QByteArray decrypt(const QByteArray& key, const QString& inputPath);
};

} // namespace RPGMakerDecrypter
