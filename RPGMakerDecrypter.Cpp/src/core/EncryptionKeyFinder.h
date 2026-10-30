#pragma once

#include <QByteArray>
#include <QString>

namespace RPGMakerDecrypter {

/// Finds the encryption key from the System.json file in an MV/MZ deployment.
class EncryptionKeyFinder
{
public:
    /// Returns the 16 byte encryption key stored inside System.json.
    /// Throws EncryptionKeyException when the key cannot be found or parsed.
    QByteArray findKey(const QString& inputPath);
};

} // namespace RPGMakerDecrypter
