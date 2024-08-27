#include <base/container/StdMapValuesEnumerator.h>
#include <base/Initializer.h>
#include <bsp-interface/di/flash.h>
#include <Flash.h>
#include <map>

base::Initializer _initializer{
    []()
    {
        DI_FlashCollection();
    }};

class Collection :
    public base::IReadOnlyCollection<std::string, bsp::IFlash *>
{
private:
    std::map<std::string, bsp::IFlash *> _map;

    void Add(bsp::IFlash *flash)
    {
        _map[flash->Name()] = flash;
    }

public:
    Collection()
    {
        Add(&hal::Flash::Instance());
    }

    virtual int Count() const override
    {
        return _map.size();
    }

    virtual bsp::IFlash *Get(std::string key) const override
    {
        auto it = _map.find(key);
        if (it == _map.end())
        {
            throw std::runtime_error{"找不到该 flash"};
        }

        return it->second;
    }

    std::shared_ptr<base::IEnumerator<bsp::IFlash *>> GetEnumerator() override
    {
        return std::shared_ptr<base::IEnumerator<bsp::IFlash *>>{
            new base::StdMapValuesEnumerator<std::string, bsp::IFlash *>{
                &_map,
            },
        };
    }
};

base::IReadOnlyCollection<std::string, bsp::IFlash *> &DI_FlashCollection()
{
    static Collection o;
    return o;
}
