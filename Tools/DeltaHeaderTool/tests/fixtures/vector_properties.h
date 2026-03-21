namespace DeltaEngine {

DCLASS()
class VectorPropsClass : public DObject
{
    DGENERATED_BODY(VectorPropsClass)

    DPROPERTY()
    std::vector<float> floatsFlat;

    DPROPERTY()
    std::vector<std::vector<float>> floatsNested;
};

}
