#include <base/container/Collection.h>
#include <bsp-interface/di/flash.h>
#include <Flash.h>
#include <map>

base::ICollection<std::string, bsp::IFlash *> const &DI_FlashCollection()
{
    class Initializer
    {
    private:
        Initializer()
        {
            Add(&hal::Flash::Instance());
        }

        void Add(bsp::IFlash *flash)
        {
            _collection.Put(flash->Name(), flash);
        }

    public:
        base::Collection<std::string, bsp::IFlash *> _collection;

        static Initializer &Instance()
        {
            class Getter : public base::SingletonGetter<Initializer>
            {
            public:
                std::unique_ptr<Initializer> Create() override
                {
                    return std::unique_ptr<Initializer>{new Initializer{}};
                }

                void Lock() override
                {
                    DI_InterruptSwitch().DisableGlobalInterrupt();
                }

                void Unlock() override
                {
                    DI_InterruptSwitch().EnableGlobalInterrupt();
                }
            };

            Getter g;
            return g.Instance();
        }
    };

    return Initializer::Instance()._collection;
}
