#include "ProjectReconstructor.h"

#include "Constants.h"
#include "CoreExceptions.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>

namespace RPGMakerDecrypter {

void ProjectReconstructor::reconstruct(const QString& deploymentPath, const QString& outputPath)
{
    if (QDir(deploymentPath).canonicalPath() == QDir(outputPath).canonicalPath()) {
        throw InvalidUsageException(
            QStringLiteral("The output directory must be different from the deployment directory. ")
            + QStringLiteral("Otherwise the deployment being reconstructed would be deleted."));
    }

    if (QFileInfo(outputPath).exists()) {
        if (!QDir(outputPath).removeRecursively()) {
            throw InvalidUsageException(
                QStringLiteral("Unable to remove the existing output directory '%1'. Make sure nothing is using it and try again.")
                    .arg(outputPath));
        }
    }

    if (!QDir().mkpath(outputPath)) {
        throw InvalidUsageException(
            QStringLiteral("Unable to create the output directory '%1'.").arg(outputPath));
    }

    const QString dataFilesPath = resolveDataFilesPath(deploymentPath);

    for (const QString& directoryName : Constants::ProjectDirectories) {
        copyDirectory(dataFilesPath + QLatin1Char('/') + directoryName,
                      outputPath + QLatin1Char('/') + directoryName);
    }

    for (const QString& fileName : Constants::ProjectFiles) {
        const QString sourceFilePath = dataFilesPath + QLatin1Char('/') + fileName;
        const QString destinationFilePath = outputPath + QLatin1Char('/') + fileName;

        if (!QFileInfo::exists(sourceFilePath)) {
            continue;
        }

        if (QFileInfo::exists(destinationFilePath)) {
            QFile::remove(destinationFilePath);
        }

        if (!QFile::copy(sourceFilePath, destinationFilePath)) {
            throw DecrypterException(
                QStringLiteral("Unable to copy file '%1' to the output directory.")
                    .arg(sourceFilePath));
        }
    }

    createProjectFile(outputPath);
}

QString ProjectReconstructor::macOSAppBundlePath(const QString& deploymentPath)
{
    const QFileInfoList macOSBundles = QDir(deploymentPath).entryInfoList(
        { QStringLiteral("*.app") }, QDir::Dirs | QDir::NoDotAndDotDot);

    if (macOSBundles.isEmpty()) {
        return {};
    }

    return macOSBundles.first().absoluteFilePath();
}

void ProjectReconstructor::copyDirectory(const QString& sourceDirPath,
                                         const QString& destinationDirPath)
{
    if (!QFileInfo(sourceDirPath).isDir()) {
        return;
    }

    QDir().mkpath(destinationDirPath);

    const QDir sourceDir(sourceDirPath);

    for (const QFileInfo& fileInfo : sourceDir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot)) {
        const QString destinationFilePath = destinationDirPath
            + QLatin1Char('/') + fileInfo.fileName();
        if (!QFile::copy(fileInfo.absoluteFilePath(), destinationFilePath)) {
            throw DecrypterException(
                QStringLiteral("Unable to copy file '%1' to the output directory.")
                    .arg(fileInfo.absoluteFilePath()));
        }
    }

    for (const QFileInfo& directoryInfo :
         sourceDir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        copyDirectory(directoryInfo.absoluteFilePath(),
                      destinationDirPath + QLatin1Char('/') + directoryInfo.fileName());
    }
}

} // namespace RPGMakerDecrypter
