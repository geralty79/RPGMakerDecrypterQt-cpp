#pragma once

#include <QString>

namespace RPGMakerDecrypter {

/// Base class for reconstructing the original project structure of an
/// RPG Maker MV or MZ deployment.
class ProjectReconstructor
{
public:
    virtual ~ProjectReconstructor() = default;

    /// Reconstructs the project from the deployment into the output directory.
    /// Existing output directory contents are removed first.
    void reconstruct(const QString& deploymentPath, const QString& outputPath);

protected:
    /// Resolves the directory that contains the actual game data files.
    virtual QString resolveDataFilesPath(const QString& deploymentPath) const = 0;

    /// Writes the project file that makes RPG Maker editors recognize the project.
    virtual void createProjectFile(const QString& outputPath) const = 0;

    /// Returns the path of a macOS app bundle directory under deploymentPath,
    /// or an empty string when the deployment has no app bundle.
    static QString macOSAppBundlePath(const QString& deploymentPath);

private:
    static void copyDirectory(const QString& sourceDirPath, const QString& destinationDirPath);
};

} // namespace RPGMakerDecrypter
