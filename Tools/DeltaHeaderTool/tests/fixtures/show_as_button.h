namespace DeltaEngine {

DCLASS()
class ShowAsButtonClass : public DObject
{
    DGENERATED_BODY(ShowAsButtonClass)
public:
    DFUNCTION(ShowAsButton)
    void DoAction();

    DFUNCTION(ShowAsButton)
    int ComputeAndReport();

    DFUNCTION(ShowAsButton)
    void BadlyAnnotated(float x);

    DFUNCTION()
    void PlainFn();

    DPROPERTY()
    float myFloat;
};

}
