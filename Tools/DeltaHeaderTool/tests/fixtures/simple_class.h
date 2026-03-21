namespace DeltaEngine {

DCLASS()
class SimpleReflectClass : public DObject
{
    DGENERATED_BODY(SimpleReflectClass)
public:
    DFUNCTION()
    void DoSomething(float x);

    DPROPERTY()
    float myFloat;

    DPROPERTY()
    int myInt;
};

}
