namespace DeltaEngine {

DENUM()
enum class TestColor : uint8_t
{
    Red = 0,
    Green = 1,
};

DCLASS()
class Foo : public DObject
{
    DGENERATED_BODY(Foo)

public:
    DPROPERTY()
    TestColor m_color;
};

}
