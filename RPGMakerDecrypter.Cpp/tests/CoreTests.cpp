// Self-test for the decryption core. Builds synthetic encrypted archives and
// deployments with the same algorithms the games use, then verifies that the
// decrypter produces the original bytes again.
//
// The encrypt and decrypt transforms are symmetric XOR operations, so the
// fixtures can be generated with the same key mathematics.

#include "ArchiveFileNameUtils.h"
#include "Constants.h"
#include "DirectoryFilesDecrypter.h"
#include "EncryptionKeyFinder.h"
#include "MVProjectReconstructor.h"
#include "ProjectGenerator.h"
#include "RGSSADV1Archive.h"
#include "RGSSADV3Archive.h"
#include "VersionFinder.h"

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QIODevice>
#include <QTemporaryDir>

#include <cstring>
#include <iostream>

using namespace RPGMakerDecrypter;

static int g_failures = 0;

#define CHECK(condition, message)                            \
    do {                                                     \
        if (!(condition)) {                                  \
            std::cerr << "FAILED: " << message << std::endl; \
            ++g_failures;                                    \
        } else {                                             \
            std::cerr << "ok: " << message << std::endl;     \
        }                                                    \
    } while (false)

static QByteArray sampleData(int size)
{
    QByteArray data(size, '\0');
    for (int i = 0; i < size; ++i) {
        // Includes byte values over 0x7F to catch signed character handling issues.
        data[i] = static_cast<char>((i * 37 + 151) % 256);
    }
    return data;
}

// Symmetric data transform, identical to RGSSADArchive::decryptFileData.
static void transformDataInPlace(QByteArray& data, quint32 key)
{
    quint32 tempKey = key;
    quint8 keyBytes[4];
    std::memcpy(keyBytes, &tempKey, sizeof(keyBytes));

    int j = 0;
    for (int i = 0; i < data.size(); ++i) {
        if (j == 4) {
            j = 0;
            tempKey *= 7;
            tempKey += 3;
            std::memcpy(keyBytes, &tempKey, sizeof(keyBytes));
        }
        data[i] = data[i] ^ static_cast<char>(keyBytes[j]);
        ++j;
    }
}

// --- RGSSAD v1 round trip (RPG Maker XP / VX) --------------------------------

static void testRGSSADV1RoundTrip(const QTemporaryDir& temporaryDirectory)
{
    const QString archivePath = temporaryDirectory.path() + QStringLiteral("/game.rgssad");
    const QByteArray expectedImageData = sampleData(1024);
    const QByteArray expectedScriptData = sampleData(377);

    {
        QFile file(archivePath);
        CHECK(file.open(QIODevice::WriteOnly), "v1 fixture file opened");

        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);

        stream.writeRawData("RGSSAD", 6);
        stream << static_cast<qint8>(0);
        stream << static_cast<qint8>(Constants::RGSSADv1Version);

        quint32 key = Constants::RGSSADv1Key;
        auto writeInteger = [&stream, &key](qint32 value) {
            stream << static_cast<quint32>(static_cast<quint32>(value) ^ key);
            key *= 7;
            key += 3;
        };
        auto writeName = [&stream, &key](const QByteArray& name) {
            for (char character : name) {
                stream << static_cast<qint8>(character ^ static_cast<char>(key & 0xFF));
                key *= 7;
                key += 3;
            }
        };

        struct Entry
        {
            QByteArray name;
            QByteArray data;
        };

        const QList<Entry> entries = {
            { QByteArray("Image.png"), expectedImageData },
            { QByteArray("Data\\Maps\\map001.rvdata"), expectedScriptData },
        };

        for (const Entry& entry : entries) {
            writeInteger(entry.name.size());
            writeName(entry.name);
            writeInteger(entry.data.size());

            // File data is encrypted with the current key state.
            QByteArray encryptedData = entry.data;
            transformDataInPlace(encryptedData, key);
            stream.writeRawData(encryptedData.constData(), encryptedData.size());
        }
    }

    CHECK(VersionFinder::findVersion(archivePath) == RPGMakerVersion::Xp,
          "v1 archive version detected from extension");

    const QString outputPath = temporaryDirectory.path() + QStringLiteral("/v1-out");
    QDir().mkpath(outputPath);

    bool extractionFailed = false;
    try {
        RGSSADV1Archive archive(archivePath);
        const QList<ArchiveEntry>& archivedFiles = archive.archivedFiles();
        CHECK(archivedFiles.size() == 2, "v1 archive has 2 entries");
        if (archivedFiles.size() == 2) {
            CHECK(archivedFiles.at(0).name == QStringLiteral("Image.png"),
                  "v1 first entry name");
            CHECK(archivedFiles.at(1).name == QStringLiteral("Data\\Maps\\map001.rvdata"),
                  "v1 second entry name with directories");
        }

        archive.extractAllFiles(outputPath, true);
    } catch (const std::exception& exception) {
        extractionFailed = true;
        std::cerr << "v1 extraction exception: " << exception.what() << std::endl;
    }
    CHECK(!extractionFailed, "v1 extraction completed without exceptions");

    QFile extractedImage(outputPath + QStringLiteral("/Image.png"));
    CHECK(extractedImage.open(QIODevice::ReadOnly), "v1 extracted image opened");
    CHECK(extractedImage.readAll() == expectedImageData, "v1 image data round trip");

    QFile extractedScript(outputPath + QStringLiteral("/Data/Maps/map001.rvdata"));
    CHECK(extractedScript.open(QIODevice::ReadOnly), "v1 extracted script opened");
    CHECK(extractedScript.readAll() == expectedScriptData, "v1 script data round trip");
}

// --- RGSSAD v3 round trip (RPG Maker VX Ace) ---------------------------------

static void testRGSSADV3RoundTrip(const QTemporaryDir& temporaryDirectory)
{
    const QString archivePath = temporaryDirectory.path() + QStringLiteral("/game.rgss3a");
    const QByteArray expectedMusicData = sampleData(2048);
    const QByteArray expectedImageData = sampleData(555);

    {
        QFile file(archivePath);
        CHECK(file.open(QIODevice::WriteOnly), "v3 fixture file opened");

        QDataStream stream(&file);
        stream.setByteOrder(QDataStream::LittleEndian);

        stream.writeRawData("RGSSAD", 6);
        stream << static_cast<qint8>(0);
        stream << static_cast<qint8>(Constants::RGSSADv3Version);

        // The stored key is multiplied by 9 and increased by 3 when read.
        const quint32 storedKey = 0x11111111;
        stream << static_cast<qint32>(storedKey);
        const quint32 key = storedKey * 9 + 3;

        auto writeInteger = [&stream, key](qint32 value) {
            stream << static_cast<quint32>(static_cast<quint32>(value) ^ key);
        };
        auto writeName = [&stream, key](const QByteArray& name) {
            quint8 keyBytes[4];
            std::memcpy(keyBytes, &key, sizeof(keyBytes));
            int j = 0;
            for (char character : name) {
                if (j == 4) {
                    j = 0;
                }
                stream << static_cast<qint8>(character ^ static_cast<char>(keyBytes[j]));
                ++j;
            }
        };

        struct Entry
        {
            QByteArray name;
            QByteArray data;
            quint32 fileKey;
        };

        const QList<Entry> entries = {
            { QByteArray("Audio\\BGM\\theme.ogg"), expectedMusicData, 0xCAFEBABE },
            { QByteArray("Graphics\\Pictures\\picture1.png"), expectedImageData, 0x12345678 },
        };

        // In the RGSSAD v3 format all directory entries are stored consecutively
        // first, terminated by an entry with an offset of zero, and only then
        // come the data blocks that the entry offsets point into.
        qint64 entryDataStart = 8 + 4; // header + stored key
        for (const Entry& entry : entries) {
            entryDataStart += 16 + entry.name.size();
        }
        entryDataStart += 16; // terminator entry

        qint64 dataOffset = entryDataStart;
        for (const Entry& entry : entries) {
            const qint32 offset = static_cast<qint32>(dataOffset);

            writeInteger(offset);
            writeInteger(static_cast<qint32>(entry.data.size()));
            // Stored value decrypts back to the plaintext file key.
            writeInteger(static_cast<qint32>(entry.fileKey));
            writeInteger(entry.name.size());
            writeName(entry.name);

            dataOffset += entry.data.size();
        }

        // An offset of zero marks the end of the directory.
        writeInteger(0);
        writeInteger(0);
        writeInteger(0);
        writeInteger(0);

        // Data blocks in the same order as the directory entries.
        for (const Entry& entry : entries) {
            // Data is encrypted with the plaintext file key.
            QByteArray encryptedData = entry.data;
            transformDataInPlace(encryptedData, entry.fileKey);
            stream.writeRawData(encryptedData.constData(), encryptedData.size());
        }
    }

    CHECK(VersionFinder::findVersion(archivePath) == RPGMakerVersion::VxAce,
          "v3 archive version detected from extension");

    const QString outputPath = temporaryDirectory.path() + QStringLiteral("/v3-out");
    QDir().mkpath(outputPath);

    bool extractionFailed = false;
    try {
        RGSSADV3Archive archive(archivePath);
        const QList<ArchiveEntry>& archivedFiles = archive.archivedFiles();
        CHECK(archivedFiles.size() == 2, "v3 archive has 2 entries");
        if (archivedFiles.size() == 2) {
            CHECK(archivedFiles.at(0).name == QStringLiteral("Audio\\BGM\\theme.ogg"),
                  "v3 first entry name");
            CHECK(archivedFiles.at(1).name == QStringLiteral("Graphics\\Pictures\\picture1.png"),
                  "v3 second entry name with directories");
        }

        archive.extractAllFiles(outputPath, true);
    } catch (const std::exception& exception) {
        extractionFailed = true;
        std::cerr << "v3 extraction exception: " << exception.what() << std::endl;
    }
    CHECK(!extractionFailed, "v3 extraction completed without exceptions");

    QFile extractedMusic(outputPath + QStringLiteral("/Audio/BGM/theme.ogg"));
    CHECK(extractedMusic.open(QIODevice::ReadOnly), "v3 extracted music opened");
    CHECK(extractedMusic.readAll() == expectedMusicData, "v3 music data round trip");

    QFile extractedPicture(outputPath + QStringLiteral("/Graphics/Pictures/picture1.png"));
    CHECK(extractedPicture.open(QIODevice::ReadOnly), "v3 extracted picture opened");
    CHECK(extractedPicture.readAll() == expectedImageData, "v3 picture data round trip");
}

// --- RPG Maker MV directory decryption ---------------------------------------

static void testMVDirectoryDecryption(const QTemporaryDir& temporaryDirectory)
{
    const QByteArray keyBytes = QByteArray::fromHex("00112233445566778899aabbccddeeff");
    const QString deploymentPath = temporaryDirectory.path() + QStringLiteral("/mv");

    QDir().mkpath(deploymentPath + QStringLiteral("/www/audio/bgm"));
    QDir().mkpath(deploymentPath + QStringLiteral("/www/img/pictures"));

    {
        QFile systemJson(deploymentPath + QStringLiteral("/www/System.json"));
        CHECK(systemJson.open(QIODevice::WriteOnly), "MV System.json opened");
        systemJson.write("{ \"encryptionKey\": \"00112233445566778899aabbccddeeff\" }");
    }

    const QByteArray expectedAudioData = sampleData(700);
    const QByteArray expectedImageData = sampleData(333);

    auto writeEncryptedMvFile = [keyBytes](const QString& filePath, const QByteArray& plainData) {
        QFile file(filePath);
        file.open(QIODevice::WriteOnly);

        // MV encodes a 16 byte fake header followed by the file data, of which the
        // first 16 bytes are additionally encrypted with the encryption key.
        file.write(QByteArray(16, '\0'));

        QByteArray encryptedPart(16, '\0');
        for (int i = 0; i < 16; ++i) {
            encryptedPart[i] = plainData[i] ^ keyBytes[i];
        }
        file.write(encryptedPart);
        file.write(plainData.mid(16));
    };

    writeEncryptedMvFile(deploymentPath + QStringLiteral("/www/audio/bgm/theme.rpgmvo"),
                         expectedAudioData);
    writeEncryptedMvFile(deploymentPath + QStringLiteral("/www/img/pictures/p1.rpgmvp"),
                         expectedImageData);

    CHECK(VersionFinder::findVersion(deploymentPath) == RPGMakerVersion::Mv,
          "MV deployment version detected");

    bool decryptionFailed = false;
    try {
        const QByteArray key = EncryptionKeyFinder().findKey(deploymentPath);
        CHECK(key == keyBytes, "MV encryption key found");

        DirectoryFilesDecrypter decrypter;
        decrypter.decrypt(key, deploymentPath, Constants::MVFileExtensionMaps, false, false);

        CHECK(QFile::exists(deploymentPath + QStringLiteral("/www/audio/bgm/theme.ogg")),
              "MV audio decrypted to .ogg");
        CHECK(QFile::exists(deploymentPath + QStringLiteral("/www/img/pictures/p1.png")),
              "MV image decrypted to .png");
        CHECK(QFile::exists(deploymentPath + QStringLiteral("/www/audio/bgm/theme.rpgmvo")),
              "MV encrypted audio kept when deleteEncrypted is false");

        QFile decryptedAudio(deploymentPath + QStringLiteral("/www/audio/bgm/theme.ogg"));
        CHECK(decryptedAudio.open(QIODevice::ReadOnly), "MV decrypted audio opened");
        CHECK(decryptedAudio.readAll() == expectedAudioData, "MV audio data round trip");

        QFile decryptedImage(deploymentPath + QStringLiteral("/www/img/pictures/p1.png"));
        CHECK(decryptedImage.open(QIODevice::ReadOnly), "MV decrypted image opened");
        CHECK(decryptedImage.readAll() == expectedImageData, "MV image data round trip");
    } catch (const std::exception& exception) {
        decryptionFailed = true;
        std::cerr << "MV decryption exception: " << exception.what() << std::endl;
    }
    CHECK(!decryptionFailed, "MV decryption completed without exceptions");
}

// --- Project reconstruction and generation -----------------------------------

static void testProjectReconstruction(const QTemporaryDir& temporaryDirectory)
{
    // Fake MV deployment with only some of the source directories present.
    const QString deploymentPath = temporaryDirectory.path() + QStringLiteral("/mv-reconstruct");
    QDir().mkpath(deploymentPath + QStringLiteral("/www/audio/bgm"));
    QDir().mkpath(deploymentPath + QStringLiteral("/www/data"));

    {
        QFile file(deploymentPath + QStringLiteral("/www/audio/bgm/theme.ogg"));
        file.open(QIODevice::WriteOnly);
        file.write(sampleData(200));
    }

    {
        QFile file(deploymentPath + QStringLiteral("/www/index.html"));
        file.open(QIODevice::WriteOnly);
        file.write("<html></html>");
    }

    const QString outputPath = temporaryDirectory.path() + QStringLiteral("/mv-reconstructed");

    bool reconstructionFailed = false;
    try {
        MVProjectReconstructor reconstructor;
        reconstructor.reconstruct(deploymentPath, outputPath);
    } catch (const std::exception& exception) {
        reconstructionFailed = true;
        std::cerr << "MV reconstruction exception: " << exception.what() << std::endl;
    }
    CHECK(!reconstructionFailed, "MV project reconstruction completed without exceptions");

    CHECK(QFile::exists(outputPath + QStringLiteral("/Game.rpgproject")),
          "MV project file created");
    CHECK(QFile::exists(outputPath + QStringLiteral("/index.html")), "MV index.html copied");
    CHECK(QFile::exists(outputPath + QStringLiteral("/audio/bgm/theme.ogg")),
          "MV audio directory copied recursively");
    CHECK(!QFileInfo(outputPath + QStringLiteral("/img")).exists(),
          "MV missing source directories are not created");

    QFile projectFile(outputPath + QStringLiteral("/Game.rpgproject"));
    CHECK(projectFile.open(QIODevice::ReadOnly), "MV project file opened");
    CHECK(projectFile.readAll() == Constants::MVProjectFileContent.toUtf8(),
          "MV project file content");

    // RGSSAD project file generation (used for reconstructed XP/VX/VX Ace projects).
    const QString xpOutputPath = temporaryDirectory.path() + QStringLiteral("/xp-project");
    QDir().mkpath(xpOutputPath);
    ProjectGenerator::generateProject(RPGMakerVersion::Xp, xpOutputPath, true);

    QFile xpProjectFile(xpOutputPath + QStringLiteral("/Game.rxproj"));
    CHECK(xpProjectFile.open(QIODevice::ReadOnly), "XP project file opened");
    CHECK(xpProjectFile.readAll() == Constants::RpgMakerXpProjectFileContent.toUtf8(),
          "XP project file content");
    CHECK(QFile::exists(xpOutputPath + QStringLiteral("/Game.ini")), "XP Game.ini created");
}

// --- Archive file name utils -------------------------------------------------

static void testArchiveFileNameUtils()
{
    // Unicode escape constants are removed from file names.
    // The name contains a literal backslash followed by 'u00e9'.
    CHECK(ArchiveFileNameUtils::getPlatformSpecificPath(
              QStringLiteral("img\\p\\test\\u00e9data.png"))
              == QStringLiteral("img/p/testdata.png"),
          "unicode escape constants removed");

    // Characters invalid in file names are stripped.
    CHECK(ArchiveFileNameUtils::getPlatformSpecificPath(QStringLiteral("a<b>c.png"))
              == QStringLiteral("abc.png"),
          "invalid characters removed");

    // Forward slashes inside a single path term are stripped as well, as they are
    // invalid file name characters in the original implementation.
    CHECK(ArchiveFileNameUtils::getPlatformSpecificPath(QStringLiteral("Graphics\\x/y.png"))
              == QStringLiteral("Graphics/xy.png"),
          "forward slash inside path term removed");

    CHECK(ArchiveFileNameUtils::getFileName(QStringLiteral("Graphics\\Pictures\\x.png"))
              == QStringLiteral("x.png"),
          "file name extracted");
}

int main()
{
    QTemporaryDir temporaryDirectory;
    if (!temporaryDirectory.isValid()) {
        std::cerr << "FAILED: temporary directory" << std::endl;
        return 1;
    }

    testArchiveFileNameUtils();
    testRGSSADV1RoundTrip(temporaryDirectory);
    testRGSSADV3RoundTrip(temporaryDirectory);
    testMVDirectoryDecryption(temporaryDirectory);
    testProjectReconstruction(temporaryDirectory);

    if (g_failures > 0) {
        std::cerr << g_failures << " test(s) FAILED" << std::endl;
        return 1;
    }

    std::cerr << "all core tests passed" << std::endl;
    return 0;
}
