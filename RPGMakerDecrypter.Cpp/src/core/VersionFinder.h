#pragma once

#include "RPGMakerVersion.h"

#include <QString>

namespace RPGMakerDecrypter {

/// Attempts to determine the RPG Maker version from the input path.
class VersionFinder
{
public:
    /// Determines the version for an input path (archive file or deployment directory).
    static RPGMakerVersion findVersion(const QString& inputPath);

    /// Attempts to determine the version from an RGSSAD archive file extension.
    static RPGMakerVersion findRGSSADVersion(const QString& filePath);

    /// Attempts to determine the RPG Maker version based on the directory structure of
    /// an MV or MZ deployment. Can detect only MV or MZ deployments.
    static RPGMakerVersion findMVMZVersion(const QString& deploymentPath);

    /// Attempts to determine the version from the RGSSAD archive header.
    /// Falls back automatically when the archive isn't named with a standard extension.
    /// Returns version Xp for version 1 archives and VxAce for version 3 archives.
    /// Returns Unknown if the file is not a valid archive.
    static RPGMakerVersion findRGSSADVersionFromHeader(const QString& filePath);

private:
    VersionFinder() = default;
};

} // namespace RPGMakerDecrypter
