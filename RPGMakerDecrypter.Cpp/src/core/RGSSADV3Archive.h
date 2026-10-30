#pragma once

#include "RGSSADArchive.h"

namespace RPGMakerDecrypter {

/// Represents RGSSAD format used in RPG Maker VX Ace.
class RGSSADV3Archive final : public RGSSADArchive
{
public:
    explicit RGSSADV3Archive(const QString& filePath);

protected:
    void readRGSSAD() override;

private:
    qint32 decryptInteger(qint32 value, quint32 key);
    QString decryptFilename(const QByteArray& encryptedName, quint32 key);
};

} // namespace RPGMakerDecrypter
