#include "ProjectGenerator.h"

#include "Constants.h"
#include "CoreExceptions.h"

#include <QDir>
#include <QFile>
#include <QIODevice>

namespace RPGMakerDecrypter {

void ProjectGenerator::generateProject(RPGMakerVersion version,
                                       const QString& outputDirectoryPath,
                                       bool overwrite)
{
    QString projectFileContent;
    QString projectFileExtension;
    QString iniFileContent;

    switch (version) {
    case RPGMakerVersion::Xp:
        projectFileContent = Constants::RpgMakerXpProjectFileContent;
        projectFileExtension = Constants::RpgMakerXpProjectFileExtension;
        iniFileContent = Constants::RpgMakerXpIniFileContents;
        break;
    case RPGMakerVersion::Vx:
        projectFileContent = Constants::RpgMakerVxProjectFileContent;
        projectFileExtension = Constants::RpgMakerVxProjectFileExtension;
        iniFileContent = Constants::RpgMakerVxIniFileContents;
        break;
    case RPGMakerVersion::VxAce:
        projectFileContent = Constants::RpgMakerVxAceProjectFileContent;
        projectFileExtension = Constants::RpgMakerVxAceProjectFileExtension;
        iniFileContent = Constants::RpgMakerVxAceIniFileContents;
        break;
    default:
        return;
    }

    const QString projectFilePath =
        outputDirectoryPath + QStringLiteral("/Game.") + projectFileExtension;
    const QString iniFilePath = outputDirectoryPath + QStringLiteral("/Game.ini");

    if (!overwrite) {
        return;
    }

    QFile projectFile(projectFilePath);
    if (!projectFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw DecrypterException(
            QStringLiteral("Unable to create the project file '%1'.").arg(projectFilePath));
    }
    projectFile.write(projectFileContent.toUtf8());
    projectFile.close();

    QFile iniFile(iniFilePath);
    if (!iniFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        throw DecrypterException(
            QStringLiteral("Unable to create the project file '%1'.").arg(iniFilePath));
    }
    iniFile.write(iniFileContent.toUtf8());
}

} // namespace RPGMakerDecrypter
