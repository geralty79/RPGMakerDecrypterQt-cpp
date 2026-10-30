#pragma once

#include <QString>
#include <QIODevice>

namespace RPGMakerDecrypter {

struct FileExtensionMapping
{
    QString encryptedExtension;
    QString realExtension;
};

namespace Constants {

// RGSSAD archive format
static const char* RGSSADHeader = "RGSSAD";
constexpr quint8 RGSSADv1Version = 1;
constexpr quint8 RGSSADv3Version = 3;
constexpr quint32 RGSSADv1Key = 0xDEADCAFE;

inline const QString RpgMakerXpArchiveExtension = QStringLiteral("rgssad");
inline const QString RpgMakerVxArchiveExtension = QStringLiteral("rgss2a");
inline const QString RpgMakerVxAceArchiveExtension = QStringLiteral("rgss3a");

// Project files for reconstructed RPG Maker XP/VX/VX Ace projects.
// Line endings are CRLF because RPG Maker editors run on Windows.
inline const QString RpgMakerXpProjectFileContent = QStringLiteral("RPGXP 1.02");
inline const QString RpgMakerVxProjectFileContent = QStringLiteral("RPGVX 1.02");
inline const QString RpgMakerVxAceProjectFileContent = QStringLiteral("RPGVXAce 1.00");

inline const QString RpgMakerXpProjectFileExtension = QStringLiteral("rxproj");
inline const QString RpgMakerVxProjectFileExtension = QStringLiteral("rvproj");
inline const QString RpgMakerVxAceProjectFileExtension = QStringLiteral("rvproj2");

inline const QString RpgMakerXpIniFileContents =
    QStringLiteral("[Game]\r\nLibrary=RGSS104E.dll\r\nScripts=Data\\Scripts.rxdata\r\nTitle=DecryptedProject\r\nRTP1=Standard\r\nRTP2=\r\nRTP3=");
inline const QString RpgMakerVxIniFileContents =
    QStringLiteral("[Game]\r\nRTP=RPGVX\r\nLibrary=RGSS202E.dll\r\nScripts=Data\\Scripts.rvdata\r\nTitle=DecryptedProject");
inline const QString RpgMakerVxAceIniFileContents =
    QStringLiteral("[Game]\r\nRTP=RPGVXAce\r\nLibrary=System\\RGSS300.dll\r\nScripts=Data\\Scripts.rvdata2\r\nTitle=DecryptedProject");

// RPG Maker MV file extension mappings
inline const QList<FileExtensionMapping> MVFileExtensionMaps = {
    { QStringLiteral(".rpgmvo"), QStringLiteral(".ogg") },
    { QStringLiteral(".rpgmvp"), QStringLiteral(".png") },
    { QStringLiteral(".rpgmvm"), QStringLiteral(".m4a") }
};

inline const QString MVProjectFileName = QStringLiteral("Game.rpgproject");
inline const QString MVProjectFileContent = QStringLiteral("RPGMV 1.6.3");

// RPG Maker MZ file extension mappings
inline const QList<FileExtensionMapping> MZFileExtensionMaps = {
    { QStringLiteral(".ogg_"), QStringLiteral(".ogg") },
    { QStringLiteral(".png_"), QStringLiteral(".png") },
    { QStringLiteral(".m4a_"), QStringLiteral(".m4a") }
};

inline const QString MZProjectFileName = QStringLiteral("game.rmmzproject");
inline const QString MZProjectFileContent = QStringLiteral("RPGMZ 1.8.0");

// Directories that should exist in a reconstructed MV/MZ project directory.
inline const QStringList ProjectDirectories = {
    QStringLiteral("audio"),
    QStringLiteral("css"),
    QStringLiteral("data"),
    QStringLiteral("effects"),
    QStringLiteral("fonts"),
    QStringLiteral("icon"),
    QStringLiteral("img"),
    QStringLiteral("js"),
    QStringLiteral("movies")
};

// Files that should exist in a reconstructed MV/MZ project directory.
inline const QStringList ProjectFiles = {
    QStringLiteral("index.html"),
    QStringLiteral("package.json")
};

inline const QString MacOSBundleDataDirectory = QStringLiteral("Contents/Resources/app.nw");

} // namespace Constants

} // namespace RPGMakerDecrypter
