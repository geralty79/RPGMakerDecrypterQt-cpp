#pragma once

#include "Constants.h"

#include <QString>

#include <functional>

namespace RPGMakerDecrypter {

/// Callback invoked after each decrypted file: (current, total, filePath).
using DirectoryProgressCallback = std::function<void(int, int, const QString&)>;
/// Callback polled during long operations; returns true when the user wants to cancel.
using CancelledCallback = std::function<bool()>;

/// Decrypts all encrypted MV/MZ files in a directory based on the provided key and
/// file extension mappings.
class DirectoryFilesDecrypter
{
public:
    /// Decrypts files found under inputPath with the given extension mappings.
    /// When deleteEncrypted is set, the encrypted source files are removed after
    /// being decrypted. When overwrite is set, existing decrypted files are replaced.
    /// progress and cancelled are optional callbacks.
    void decrypt(const QByteArray& key,
                 const QString& inputPath,
                 const QList<FileExtensionMapping>& fileExtensionMaps,
                 bool deleteEncrypted,
                 bool overwrite,
                 const DirectoryProgressCallback& progress = {},
                 const CancelledCallback& cancelled = {});
};

} // namespace RPGMakerDecrypter
