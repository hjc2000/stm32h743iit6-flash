#pragma once
#include <atomic>
#include <base/di/SingletonGetter.h>
#include <base/LockGuard.h>
#include <bsp-interface/di/interrupt.h>
#include <bsp-interface/flash/IFlash.h>
#include <hal.h>
#include <task/BinarySemaphore.h>

extern "C"
{
    void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue);
    void HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue);
}

namespace hal
{
    class Flash : public bsp::IFlash
    {
    private:
        Flash();

        static uint32_t SectorIndexToDefine(int32_t index);
        friend void ::HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue);
        friend void ::HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue);

        std::atomic_bool _operation_failed = false;
        task::BinarySemaphore _operation_completed;

    public:
        static Flash &Instance()
        {
            class Getter : public base::SingletonGetter<Flash>
            {
            public:
                std::unique_ptr<Flash> Create() override
                {
                    return std::unique_ptr<Flash>{new Flash{}};
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

        /// @brief flash 的名称。
        /// @return
        std::string Name() override
        {
            // 片内 flash
            return "internal-flash";
        }

        void Lock() override;
        void Unlock() override;

        /// @brief 获取此 flash 的 bank 数量。
        /// @return
        int32_t BankCount() const override
        {
            return 2;
        }

        /// @brief 获取指定 bank 的扇区数量。
        /// @param bank_index
        /// @return
        int32_t GetBankSectorCount(int32_t bank_index) const override
        {
            return 8;
        }

        /// @brief 获取指定 bank 的基地址。
        /// @param bank_index
        /// @return
        size_t GetBankBaseAddress(int32_t bank_index) const override;

        /// @brief 获取指定 bank 的大小。单位：字节。
        /// @param bank_index
        /// @return
        virtual size_t GetBankSize(int32_t bank_index) const override;

        /// @brief flash 的最小编程单位。单位：字节。
        /// @note 最小单位是一次编程必须写入这么多字节，即使要写入的数据没有这么多，在一次
        /// 写入后，整个单位大小的区域都无法再次写入了，除非擦除整个扇区。
        /// @return 返回此 flash 编程的最小单位。
        int32_t MinProgrammingUnit() const override
        {
            return 32;
        }

#pragma region 擦除
        /// @brief 擦除一整个 bank。
        /// @note stm32h743 有 2 个 bank。典型用法是：bank1 用来存放程序，bank2 用来存放数据。
        /// @param bank_index
        void EraseBank(int32_t bank_index) override;

        void EraseBank_NoIT(int32_t bank_index);

        /// @brief 擦除指定 bank 的指定扇区。
        /// @param bank_index
        /// @param sector_index 要擦除的扇区索引。
        void EraseSector(int32_t bank_index, int32_t sector_index);
        using bsp::IFlash::EraseSector;

        void EraseSector_NoIT(int32_t bank_index, int32_t sector_index);
#pragma endregion

        void ReadBuffer(int32_t bank_index, size_t addr, uint8_t *buffer, int32_t count) override;

#pragma region 编程
        void Program(int32_t bank_index, size_t addr, uint8_t const *buffer) override;

        void Program_NoIT(int32_t bank_index, size_t addr, uint8_t const *buffer);
#pragma endregion
    };
} // namespace hal
