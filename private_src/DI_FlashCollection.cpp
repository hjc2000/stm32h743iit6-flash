#include <base/container/Dictionary.h>
#include <bsp-interface/di/flash.h>
#include <bsp-interface/TaskSingletonGetter.h>
#include <Flash.h>
#include <map>

namespace
{
    class Initializer
    {
    private:
        void Add(bsp::IFlash *flash)
        {
            _dic.Add(flash->Name(), flash);
        }

    public:
        Initializer()
        {
            Add(&bsp::Flash::Instance());
        }

        base::Dictionary<std::string, bsp::IFlash *> _dic;
    };

    class Getter :
        public bsp::TaskSingletonGetter<Initializer>
    {
    public:
        std::unique_ptr<Initializer> Create() override
        {
            return std::unique_ptr<Initializer>{new Initializer{}};
        }
    };
} // namespace

base::IDictionary<std::string, bsp::IFlash *> const &DI_FlashCollection()
{
    Getter g;
    return g.Instance()._dic;
}
