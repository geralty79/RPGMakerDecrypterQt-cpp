#include "DirectoryFilesDecrypter.h"

#include "CoreExceptions.h"
#include "FileDecrypter.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>

namespace RPGMakerDecrypter {

void DirectoryFilesDecrypter::decrypt(const QByteArray& key,
                                      const QString& inputPath,
                                      const QList<FileExtensionMapping>& fileExtensionMaps,
                                      bool deleteEncrypted,
                                      bool overwrite,
                                      const DirectoryProgressCallback& progress,
                                      const CancelledCallback& cancelled)
{
    // Collect all encrypted files by matching the encrypted extensions.
    // Extensions are matched case-insensitively, like on Windows.
    QStringList encryptedFiles;
    for (const FileExtensionMapping& mapping : fileExtensionMaps) {
        QDirIterator iterator(inputPath, QDir::Files, QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString filePath = iterator.next();
            if (filePath.endsWith(mapping.encryptedExtension, Qt::CaseInsensitive)) {
                encryptedFiles.append(filePath);
            }
        }
    }

    const int total = encryptedFiles.size();

    FileDecrypter fileDecrypter;

    int current = 0;
    for (const QString& encryptedFilePath : encryptedFiles) {
        if (cancelled && cancelled()) {
            throw OperationCancelledException();
        }

        QFileInfo encryptedFileInfo(encryptedFilePath);

        // Find the mapping for the extension of this file.
        const FileExtensionMapping* mapping = nullptr;
        for (const FileExtensionMapping& candidate : fileExtensionMaps) {
            if (encryptedFileInfo.fileName().endsWith(candidate.encryptedExtension, Qt::CaseInsensitive)) {
                mapping = &candidate;
                break;
            }
        }

        if (mapping == nullptr) {
            continue;
        }

        // Encrypted files have a changed extension. Map it back to the original
        // extension to create the target path of the decrypted file.
        const QString fileName = encryptedFileInfo.fileName();
        const QString fileNameWithoutExtension =
            fileName.left(fileName.size() - mapping->encryptedExtension.size());
        const QString targetFilePath = encryptedFileInfo.absolutePath()
            + QLatin1Char('/') + fileNameWithoutExtension + mapping->realExtension;

        if (!overwrite && QFileInfo::exists(targetFilePath)) {
            ++current;
            if (progress) {
                progress(current, total, targetFilePath);
            }
            continue;
        }

        const QByteArray decryptedData = fileDecrypter.decrypt(key, encryptedFilePath);

        if (decryptedData.isEmpty()) {
            ++current;
            if (progress) {
                progress(current, total, targetFilePath);
            }
            continue;
        }

        QFile targetFile(targetFilePath);
        if (!targetFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            throw DecrypterException(
                QStringLiteral("Unable to write file '%1'. The file might be open in another program or the output directory might not be writable.")
                    .arg(targetFilePath));
        }
        targetFile.write(decryptedData);
        targetFile.close();

        if (deleteEncrypted) {
            QFile::remove(encryptedFilePath);
        }

        ++current;
        if (progress) {
            progress(current, total, targetFilePath);
        }
    }
}

} // namespace RPGMakerDecrypter
