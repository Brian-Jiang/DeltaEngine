namespace DeltaEngine {

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlain);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOneInt, int, Value);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnTwoArgs, int, First, float, Second);
DECLARE_DYNAMIC_DELEGATE(FOnExecutePlain);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnExecuteInt, int, Value);

DCLASS()
class DelegatePropertyHost : public DObject
{
    DGENERATED_BODY(DelegatePropertyHost)
public:
    DPROPERTY()
    FDynamicMulticastDelegate m_plainDelegate;

    DPROPERTY()
    FOnOneInt m_onOneInt;
};

}
