#include "MZProjectReconstructor.h"

#include "Constants.h"
#include "CoreExceptions.h"

#include <QDir>
#include <QFile>
#include <QIODevice>

namespace RPGMakerDecrypter {

QString MZProjectReconstructor::resolveDataFilesPath(const QString& deploymentPath) const
{
    // MacOS deployments have a different structure that needs to be handled separately.
    const QString macOSBundle = macOSAppBundlePath(deploymentPath);
    if (!macOSBundle.isEmpty()) {
        return macOSBundle + QLatin1Char('/') + Constants::MacOSBundleDataDirectory;
    }

    // Windows, Linux, Web and Android/iOS deployments have all files in the
    // root of the deployment.
    return deploymentPath;
}

void MZProjectReconstructor::createProjectFile(const QString& outputPath) const
{
    const QString projectFilePath = outputPath + QLatin1Char('/') + Constants::MZProjectFileName;
    QFile projectFile(projectFilePath);
    if (!projectFile.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        throw DecrypterException(
            QStringLiteral("Unable to create the project file '%1'.").arg(projectFilePath));
    }
    projectFile.write(Constants::MZProjectFileContent.toUtf8());
}

} // namespace RPGMakerDecrypter
