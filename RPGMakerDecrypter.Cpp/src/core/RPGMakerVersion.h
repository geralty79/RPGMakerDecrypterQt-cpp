#pragma once

#include <QString>

namespace RPGMakerDecrypter {

/// RPG Maker versions supported by the decrypter.
enum class RPGMakerVersion
{
    Unknown,
    Xp,
    Vx,
    VxAce,
    Mv,
    Mz
};

inline QString toDisplayString(RPGMakerVersion version)
{
    switch (version) {
    case RPGMakerVersion::Xp:
        return QStringLiteral("RPG Maker XP");
    case RPGMakerVersion::Vx:
        return QStringLiteral("RPG Maker VX");
    case RPGMakerVersion::VxAce:
        return QStringLiteral("RPG Maker VX Ace");
    case RPGMakerVersion::Mv:
        return QStringLiteral("RPG Maker MV");
    case RPGMakerVersion::Mz:
        return QStringLiteral("RPG Maker MZ");
    case RPGMakerVersion::Unknown:
        break;
    }
    return QStringLiteral("Unknown");
}

} // namespace RPGMakerDecrypter
