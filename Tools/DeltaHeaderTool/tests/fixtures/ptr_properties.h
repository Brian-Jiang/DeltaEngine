namespace DeltaEngine {

DCLASS()
class PtrPropsClass : public DObject
{
    DGENERATED_BODY(PtrPropsClass)

    DPROPERTY()
    DObject* rawPtr;
};

}
