
/****************************************************************************

**

** Copyright (C) 2007-2009 Kevin Clague. All rights reserved.

** Copyright (C) 2015 - 2025 Trevor SANDY. All rights reserved.

**

** This file may be used under the terms of the GNU General Public

** License version 2.0 as published by the Free Software Foundation

** and appearing in the file LICENSE.GPL included in the packaging of

** this file.  Please review the following information to ensure GNU

** General Public Licensing requirements will be met:

** http://www.trolltech.com/products/qt/opensource.html

**

** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE

** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

**

****************************************************************************/

#include <QDir>
#include <QString>
#include "paths.h"
#include "lpub_preferences.h"
#include "version.h"

Paths paths;

QString Paths::lpubDir          = QLatin1String(VER_PRODUCTNAME_STR);
QString Paths::tmpDir           = lpubDir + QDir::separator() + QLatin1String("tmp");
QString Paths::assemDir         = lpubDir + QDir::separator() + QLatin1String("assem");
QString Paths::partsDir         = lpubDir + QDir::separator() + QLatin1String("parts");
QString Paths::bomDir           = lpubDir + QDir::separator() + QLatin1String("bom");
QString Paths::submodelDir      = lpubDir + QDir::separator() + QLatin1String("submodels");
QString Paths::povrayRenderDir  = lpubDir + QDir::separator() + QLatin1String("povray");
QString Paths::blenderRenderDir = lpubDir + QDir::separator() + QLatin1String("blender");
QString Paths::htmlStepsDir     = lpubDir + QDir::separator() + QLatin1String("htmlsteps");
/*** LPub3D Mod - ASCII scratch for POV-Ray (LPub3D-Trace rejects non-ASCII paths) ***/
// LPub3D-Trace opens +I/+O paths itself and only accepts ASCII. A Studio model
// leaves its name in the working directory ("IO Enhancement", "测试文件 2"),
// which made every render fail, so scene files and rendered images are staged
// under the system temp directory, which contains no spaces and no non-ASCII
// characters on macOS. RendererScratchDir() is shared by the scene writers
// (step.cpp, pli.cpp) and the renderers (render.cpp) so both sides agree on the
// location without changing the working directory everything else relies on.
QString Paths::rendererScratchDir()
{
    static const QString dir = QDir::tempPath() + QDir::separator() + QLatin1String("myLPub3D-render");
    QDir().mkpath(dir);
    return dir;
}
/*** LPub3D Mod end ***/

QString Paths::logsDir          = QLatin1String("logs");
QString Paths::extrasDir        = QLatin1String("extras");
QString Paths::libraryDir       = QLatin1String("libraries");

QString Paths::customDir;
QString Paths::customPartDir;
QString Paths::customSubDir;
QString Paths::customTextureDir;
QString Paths::customPrimDir;
QString Paths::customPrim8Dir;
QString Paths::customPrim48Dir;

QStringList Paths::customDirs;

void Paths::mkPovrayDir() {
    QDir dir;
    dir.mkdir(povrayRenderDir);
}

void Paths::mkBlenderDir() {
    QDir dir;
    dir.mkdir(blenderRenderDir);
}

void Paths::mkDirs() {
    QDir dir;
    dir.mkdir(lpubDir);
    dir.mkdir(tmpDir);
    dir.mkdir(bomDir);
    dir.mkdir(assemDir);
    dir.mkdir(partsDir);
    dir.mkdir(submodelDir);
}

void Paths::mkCustomDirs() {
    customDir        = Preferences::validLDrawLibrary + QLatin1String("custom");
    customPartDir    = customDir + QDir::separator() + QLatin1String("parts");
    customSubDir     = customPartDir + QDir::separator() + QLatin1String("s");
    customTextureDir = customPartDir + QDir::separator() + QLatin1String("textures");
    customPrimDir    = customDir + QDir::separator() + QLatin1String("p");
    customPrim8Dir   = customPrimDir + QDir::separator() + QLatin1String("8");
    customPrim48Dir  = customPrimDir + QDir::separator() + QLatin1String("48");
    const static QString lpubDataPath = Preferences::lpubDataPath + QDir::separator();
    customDirs << lpubDataPath + customPartDir
               << lpubDataPath + customSubDir
               << lpubDataPath + customTextureDir
               << lpubDataPath + customPrimDir
               << lpubDataPath + customPrim8Dir
               << lpubDataPath + customPrim48Dir;
    QDir dir;
    for (QString &path : customDirs) {
        if (!dir.exists(path)) dir.mkdir(path);
    }
}
