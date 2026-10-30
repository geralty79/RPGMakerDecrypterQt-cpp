#pragma once

#include <QString>

#include <QtGlobal>

namespace RPGMakerDecrypter {

/// A single file inside an RGSSAD archive.
struct ArchiveEntry
{
    QString name;      ///< Decrypted file name with original directory structure.
    qint64 offset = 0; ///< Byte offset of the file data inside the archive.
    qint64 size = 0;   ///< Size of the file data in bytes.
    quint32 key = 0;   ///< Key used to decrypt the file data.
};

} // namespace RPGMakerDecrypter
