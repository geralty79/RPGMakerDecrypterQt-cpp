#include "ArchiveFileNameUtils.h"

#include <QRegularExpression>

namespace RPGMakerDecrypter {

QString ArchiveFileNameUtils::getPlatformSpecificPath(const QString& archiveName)
{
    // Matches unicode escape constants such as "\u00e9". The cleaning is done on the
    // full name, before splitting into path parts, because the backslash that starts
    // the escape sequence would otherwise be interpreted as a path separator.
    static const QRegularExpression unicodeConstantRegex(
        QStringLiteral("\\\\[uU][0-9A-Fa-f]{4}"));

    // Characters that are invalid in file names on target platforms
    // (mirrors what Path.GetInvalidFileNameChars() would filter out).
    static const QString invalidFileNameCharacters = QStringLiteral("\"<>|:*?\\/");
    QString cleanedName = archiveName;
    cleanedName.remove(unicodeConstantRegex);

    // Paths in RGSSAD file names always use Windows style separators.
    const QStringList pathParts = cleanedName.split(QLatin1Char('\\'));

    QString result;
    result.reserve(archiveName.size());

    bool firstPart = true;
    for (const QString& pathPart : pathParts) {
        if (!firstPart) {
            result += QLatin1Char('/');
        }
        firstPart = false;

        QString cleanedPart = pathPart;
        cleanedPart.remove(unicodeConstantRegex);

        for (const QChar& invalidCharacter : invalidFileNameCharacters) {
            cleanedPart.remove(invalidCharacter);
        }

        // Mirrors Path.GetInvalidFileNameChars(), which also excludes ASCII control characters.
        QString controlCharacterFreePart;
        for (const QChar& character : cleanedPart) {
            if (character.unicode() < 0x20) {
                continue;
            }
            controlCharacterFreePart.append(character);
        }
        cleanedPart = controlCharacterFreePart;

        result += cleanedPart;
    }

    return result;
}

QString ArchiveFileNameUtils::getFileName(const QString& archiveName)
{
    const QStringList pathParts = archiveName.split(QLatin1Char('\\'));
    return pathParts.last();
}

} // namespace RPGMakerDecrypter
