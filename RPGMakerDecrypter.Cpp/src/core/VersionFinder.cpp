#include "VersionFinder.h"

#include "Constants.h"
#include "RGSSADArchive.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>

namespace RPGMakerDecrypter {

RPGMakerVersion VersionFinder::findVersion(const QString& inputPath)
{
    const QFileInfo inputInfo(inputPath);

    if (!inputInfo.exists()) {
        return RPGMakerVersion::Unknown;
    }

    if (inputInfo.isFile()) {
        RPGMakerVersion version = findRGSSADVersion(inputPath);
        if (version == RPGMakerVersion::Unknown) {
            // The archive might just have been renamed, try the header next.
            version = findRGSSADVersionFromHeader(inputPath);
        }
        return version;
    }

    return findMVMZVersion(inputPath);
}

RPGMakerVersion VersionFinder::findRGSSADVersion(const QString& filePath)
{
    return RGSSADArchive::getRPGMakerVersion(filePath);
}

RPGMakerVersion VersionFinder::findRGSSADVersionFromHeader(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return RPGMakerVersion::Unknown;
    }

    QDataStream stream(&file);
    stream.setByteOrder(QDataStream::LittleEndian);

    try {
        const quint8 version = RGSSADArchive::readHeaderVersion(stream);
        if (version == Constants::RGSSADv1Version) {
            return RPGMakerVersion::Xp;
        }
        if (version == Constants::RGSSADv3Version) {
            return RPGMakerVersion::VxAce;
        }
    } catch (const std::exception&) {
        return RPGMakerVersion::Unknown;
    }

    return RPGMakerVersion::Unknown;
}

RPGMakerVersion VersionFinder::findMVMZVersion(const QString& deploymentPath)
{
    const QDir deploymentDir(deploymentPath);

    // MV deployments have a separate www-directory that contains the data.
    if (deploymentDir.exists(QStringLiteral("www"))) {
        return RPGMakerVersion::Mv;
    }

    // MacOS deployments look the same for both versions, but have a slightly
    // different internal structure.
    const QFileInfoList macOSBundles = deploymentDir.entryInfoList(
        { QStringLiteral("*.app") }, QDir::Dirs | QDir::NoDotAndDotDot);
    if (!macOSBundles.isEmpty()) {
        // The Framework directory is missing from MV, it only exists in MZ.
        if (QDir(macOSBundles.first().absoluteFilePath() + QStringLiteral("/Contents/Frameworks")).exists()) {
            return RPGMakerVersion::Mz;
        }

        return RPGMakerVersion::Mv;
    }

    // MZ deployments have the files in the root directory.
    return RPGMakerVersion::Mz;
}

} // namespace RPGMakerDecrypter
