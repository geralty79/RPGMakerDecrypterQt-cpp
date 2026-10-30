#include "DecryptWorker.h"

#include "DirectoryFilesDecrypter.h"
#include "EncryptionKeyFinder.h"
#include "Constants.h"
#include "CoreExceptions.h"
#include "MZProjectReconstructor.h"
#include "MVProjectReconstructor.h"
#include "ProjectGenerator.h"
#include "RGSSADArchive.h"
#include "VersionFinder.h"

#include <QDir>
#include <QFileInfo>

namespace RPGMakerDecrypter {

DecryptWorker::DecryptWorker(QObject* parent)
    : QObject(parent)
{
    m_cancelRequested.store(false);
}

void DecryptWorker::cancel()
{
    m_cancelRequested.store(true);
}

bool DecryptWorker::cancellationRequested()
{
    return m_cancelRequested.load();
}

void DecryptWorker::run(const DecryptRequest& request)
{
    m_cancelRequested.store(false);

    emit progress(0, 0, QString());

    try {
        const RPGMakerVersion version = VersionFinder::findVersion(request.inputPath);

        switch (version) {
        case RPGMakerVersion::Xp:
        case RPGMakerVersion::Vx:
        case RPGMakerVersion::VxAce:
            runArchiveOperation(request, version);
            break;
        case RPGMakerVersion::Mv:
        case RPGMakerVersion::Mz:
            runMVMZOperation(request, version);
            break;
        case RPGMakerVersion::Unknown:
        default:
            throw InvalidUsageException(
                QStringLiteral("Unable to determine the RPG Maker version. ")
                + QStringLiteral("Please rename the RGSSAD file with an extension corresponding to the version: ")
                + QStringLiteral("XP: .rgssad, VX: .rgss2a, VX Ace: .rgss3a ")
                + QStringLiteral("or point to an MV or MZ directory."));
            break;
        }
    } catch (const OperationCancelledException&) {
        emit finished(false, tr("The operation was cancelled."));
        return;
    } catch (const DecrypterException& exception) {
        emit finished(false, QString::fromStdString(exception.what()));
        return;
    } catch (const std::exception& exception) {
        emit finished(false,
                      tr("Unexpected error happened while trying to extract the archive: %1")
                          .arg(QString::fromUtf8(exception.what())));
        return;
    }

    emit finished(true, tr("Done."));
}

void DecryptWorker::runArchiveOperation(const DecryptRequest& request, RPGMakerVersion version)
{
    const QFileInfo inputInfo(request.inputPath);

    std::unique_ptr<RGSSADArchive> archive = RGSSADArchive::open(request.inputPath);

    // When no output directory is specified, the files are extracted next to the archive.
    QString outputDirectoryPath = request.outputPath;
    if (outputDirectoryPath.isEmpty()) {
        outputDirectoryPath = inputInfo.absolutePath();
    }

    if (QFileInfo(outputDirectoryPath).isDir() && !request.overwrite) {
        throw InvalidUsageException(
            QStringLiteral("Output directory already exists. Please specify a different path or enable the overwriting of existing files."));
    }

    QDir().mkpath(outputDirectoryPath);

    emit message(tr("RPG Maker version: %1").arg(toDisplayString(version)));
    emit message(tr("Extracting %1 file(s) to '%2'...")
                     .arg(archive->archivedFiles().size())
                     .arg(QDir::toNativeSeparators(outputDirectoryPath)));

    archive->extractAllFiles(
        outputDirectoryPath,
        request.overwrite,
        [this](int current, int total, const QString& fileName) {
            emit progress(current, total, fileName);
        },
        [this]() { return m_cancelRequested.load(); });

    emit message(tr("Extracted %1 file(s) to '%2'.")
                     .arg(archive->archivedFiles().size())
                     .arg(QDir::toNativeSeparators(outputDirectoryPath)));

    if (request.reconstructProject) {
        // Project files for the editor are only generated when the output
        // directory is different from the archive directory, matching the
        // behavior of the original CLI. Otherwise the files that are deployed
        // with the game would be replaced.
        const QString outputCanonicalPath = QDir(outputDirectoryPath).canonicalPath();
        const QString archiveCanonicalPath = QDir(inputInfo.absolutePath()).canonicalPath();
        const bool outputSameAsArchivePath = outputCanonicalPath == archiveCanonicalPath;

        ProjectGenerator::generateProject(version, outputDirectoryPath, !outputSameAsArchivePath);

        if (!outputSameAsArchivePath) {
            emit message(tr("Project files have been generated to '%1'.")
                             .arg(QDir::toNativeSeparators(outputDirectoryPath)));
        }
    }
}

void DecryptWorker::runMVMZOperation(const DecryptRequest& request, RPGMakerVersion version)
{
    if (request.reconstructProject && request.outputPath.trimmed().isEmpty()) {
        throw InvalidUsageException(
            QStringLiteral("MV and MZ project reconstruction requires an output path."));
    }

    if (!request.outputPath.isEmpty() && QFileInfo(request.outputPath).isDir() && !request.overwrite) {
        throw InvalidUsageException(
            QStringLiteral("Output directory already exists. Please specify a different path or enable the overwriting of existing files."));
    }

    QString workingDirectoryPath = request.inputPath;
    bool deleteEncrypted = false;

    if (request.reconstructProject) {
        emit message(tr("Reconstructing project files to '%1'...")
                         .arg(QDir::toNativeSeparators(request.outputPath)));

        switch (version) {
        case RPGMakerVersion::Mv:
            MVProjectReconstructor().reconstruct(request.inputPath, request.outputPath);
            break;
        case RPGMakerVersion::Mz:
            MZProjectReconstructor().reconstruct(request.inputPath, request.outputPath);
            break;
        default:
            break;
        }

        // Change working directory to the project directory where all the
        // necessary files are located too.
        workingDirectoryPath = request.outputPath;
        deleteEncrypted = true;
    }

    emit message(tr("Searching for the encryption key..."));

    const QByteArray encryptionKey = EncryptionKeyFinder().findKey(workingDirectoryPath);

    emit message(tr("Decrypting files in '%1'...")
                     .arg(QDir::toNativeSeparators(workingDirectoryPath)));

    const QList<FileExtensionMapping> fileExtensionMaps =
        (version == RPGMakerVersion::Mv)
            ? Constants::MVFileExtensionMaps
            : Constants::MZFileExtensionMaps;

    DirectoryFilesDecrypter decrypter;

    decrypter.decrypt(
        encryptionKey,
        workingDirectoryPath,
        fileExtensionMaps,
        deleteEncrypted,
        request.overwrite,
        [this](int current, int total, const QString& currentItem) {
            emit progress(current, total, currentItem);
        },
        [this]() { return m_cancelRequested.load(); });

    if (deleteEncrypted) {
        emit message(tr("Encrypted files have been removed from the output directory."));
    }
}

} // namespace RPGMakerDecrypter
