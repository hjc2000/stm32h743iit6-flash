#include <base/container/Collection.h>
#include <base/Initializer.h>
#include <bsp-interface/di/flash.h>
#include <Flash.h>
#include <map>

base::Initializer _initializer{
    []()
    {
        DI_FlashCollection();
    }};

class Collection
{
public:
    Collection()
    {
        Add(&hal::Flash::Instance());
    }

    base::Collection<std::string, bsp::IFlash *> _collection;

    void Add(bsp::IFlash *flash)
    {
        _collection.Put(flash->Name(), flash);
    }
};

base::ICollection<std::string, bsp::IFlash *> const &DI_FlashCollection()
{
    static Collection o;
    return o._collection;
}
