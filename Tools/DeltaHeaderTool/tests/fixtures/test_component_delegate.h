namespace DeltaEngine {

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTestComponentEvent, int, Value);

DCLASS()
class TestComponent : public DComponent
{
    DGENERATED_BODY(TestComponent)

public:
    DFUNCTION()
    void OnTestEventReceived(int value);

private:
    DPROPERTY()
    FTestComponentEvent OnTestEvent;
};

}
