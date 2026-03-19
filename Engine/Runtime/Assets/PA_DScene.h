#pragma once

#include "EngineIncludes.h"

#include <string>

#include "Assets/DPrimaryAsset.h"

#include "PA_DScene.generated.h"

DELTA_ENGINE_NS_BEGIN

class DScene;

DCLASS()
class DELTAENGINE_API PA_DScene : public DPrimaryAsset
{
    DGENERATED_BODY(PA_DScene)

public:
    static PA_DScene* Create(const std::string& sceneName);
    DScene* GetScene() const;
};

DELTA_ENGINE_NS_END
