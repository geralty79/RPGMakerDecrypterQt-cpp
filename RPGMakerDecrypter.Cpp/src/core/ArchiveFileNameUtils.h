#pragma once

#include <QString>

namespace RPGMakerDecrypter {

/// Utility for converting archive-internal file names into safe, platform specific paths.
class ArchiveFileNameUtils
{
public:
    /// Converts the decrypted file name into a path suitable for the running platform.
    /// The archive always stores Windows style separators. Unicode escape constants
    /// (e.g. "\u00e9") and characters that are invalid in file names are removed.
    static QString getPlatformSpecificPath(const QString& archiveName);

    /// Returns only the file name part of a decrypted archive file name.
    static QString getFileName(const QString& archiveName);

private:
    ArchiveFileNameUtils() = default;
};

} // namespace RPGMakerDecrypter
