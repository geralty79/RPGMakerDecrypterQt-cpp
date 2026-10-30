#pragma once

#include "RGSSADArchive.h"

namespace RPGMakerDecrypter {

/// Represents RGSSAD format used in RPG Maker XP and VX.
class RGSSADV1Archive final : public RGSSADArchive
{
public:
    explicit RGSSADV1Archive(const QString& filePath);

protected:
    void readRGSSAD() override;

private:
    qint32 decryptInteger(qint32 value, quint32& key);
    QString decryptFilename(const QByteArray& encryptedName, quint32& key);
};

} // namespace RPGMakerDecrypter
