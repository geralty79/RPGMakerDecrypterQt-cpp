#pragma once

#include "RPGMakerVersion.h"

#include <QString>

namespace RPGMakerDecrypter {

/// Generates the RPG Maker project file and ini for reconstructed XP/VX/VX Ace projects.
class ProjectGenerator
{
public:
    /// Generates the project file and ini for the given RPG Maker version into
    /// the output directory. Files are only written when overwrite is set.
    static void generateProject(RPGMakerVersion version,
                                const QString& outputDirectoryPath,
                                bool overwrite);
};

} // namespace RPGMakerDecrypter
