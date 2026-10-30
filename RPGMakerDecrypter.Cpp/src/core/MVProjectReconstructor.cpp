#include "MVProjectReconstructor.h"

#include "Constants.h"
#include "CoreExceptions.h"

#include <QDir>
#include <QFile>
#include <QIODevice>

namespace RPGMakerDecrypter {

QString MVProjectReconstructor::resolveDataFilesPath(const QString& deploymentPath) const
{
    // MacOS deployments have a different structure that needs to be handled separately.
    const QString macOSBundle = macOSAppBundlePath(deploymentPath);
    if (!macOSBundle.isEmpty()) {
        return macOSBundle + QLatin1Char('/') + Constants::MacOSBundleDataDirectory;
    }

    // Windows, Linux, Web and Android/iOS deployments have a separate www-directory
    // that essentially contains the project files.
    const QString wwwPath = deploymentPath + QStringLiteral("/www");
    if (QDir(wwwPath).exists()) {
        return wwwPath;
    }

    return deploymentPath;
}

void MVProjectReconstructor::createProjectFile(const QString& outputPath) const
{
    const QString projectFilePath = outputPath + QLatin1Char('/') + Constants::MVProjectFileName;
    QFile projectFile(projectFilePath);
    if (!projectFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        throw DecrypterException(
            QStringLiteral("Unable to create the project file '%1'.").arg(projectFilePath));
    }
    projectFile.write(Constants::MVProjectFileContent.toUtf8());
}

} // namespace RPGMakerDecrypter
