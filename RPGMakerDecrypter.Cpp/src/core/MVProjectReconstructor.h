#pragma once

#include "ProjectReconstructor.h"

namespace RPGMakerDecrypter {

/// Reconstructs an RPG Maker MV project from a deployment.
class MVProjectReconstructor final : public ProjectReconstructor
{
protected:
    QString resolveDataFilesPath(const QString& deploymentPath) const override;
    void createProjectFile(const QString& outputPath) const override;
};

} // namespace RPGMakerDecrypter
