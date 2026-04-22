namespace DeltaEngine {

DCLASS()
class DELTAENGINE_API ApiMacroBase : public DObject
{
    DGENERATED_BODY(ApiMacroBase)
public:
    DPROPERTY()
    float myFloat;
};

DCLASS()
class DELTAENGINE_API ApiMacroDerived : public ApiMacroBase
{
    DGENERATED_BODY(ApiMacroDerived)
};

}
