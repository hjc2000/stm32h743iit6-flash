#pragma once
#include <atomic>
#include <base/define.h>
#include <base/di/SingletonGetter.h>
#include <base/LockGuard.h>
#include <bsp-interface/di/interrupt.h>
#include <bsp-interface/di/task.h>
#include <bsp-interface/flash/IFlash.h>
#include <hal.h>

extern "C"
{
    void HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue);
    void HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue);
}

namespace bsp
{
    class Flash :
        public bsp::IFlash
    {
    private:
        Flash();

        static_function uint32_t SectorIndexToDefine(int32_t index);
        friend void ::HAL_FLASH_EndOfOperationCallback(uint32_t ReturnValue);
        friend void ::HAL_FLASH_OperationErrorCallback(uint32_t ReturnValue);

        std::atomic_bool _operation_failed = false;
        std::shared_ptr<bsp::IBinarySemaphore> _operation_completed = DICreate_BinarySemaphore();

    public:
        static_function Flash &Instance();

        /// @brief flash 的名称。
        /// @return
        std::string Name() override
        {
            // 片内 flash
            return "internal-flash";
        }

        void Lock() override;
        void Unlock() override;

#pragma region flash 参数

        /// @brief 获取此 flash 的 bank 数量。
        /// @return
        int32_t BankCount() const override
        {
            return 2;
        }

        /// @brief 一个扇区的大小。单位：字节。
        /// @return
        size_t SectorSize() const override
        {
            return static_cast<size_t>(128) << 10;
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

        /// @brief flash 的最小编程单位。单位：字节。
        /// @note 最小单位是一次编程必须写入这么多字节，即使要写入的数据没有这么多，在一次
        /// 写入后，整个单位大小的区域都无法再次写入了，除非擦除整个扇区。
        /// @return 返回此 flash 编程的最小单位。
        int32_t MinProgrammingUnit() const override
        {
            return 32;
        }

#pragma endregion

#pragma region 擦除
        /// @brief 擦除一整个 bank。
        /// @note stm32h743 有 2 个 bank。典型用法是：bank1 用来存放程序，bank2 用来存放数据。
        /// @warning bank_index 为 0 表示 bank1，索引为 1 表示 bank2。
        /// @param bank_index
        void EraseBank(int32_t bank_index) override;

        /// @brief 擦除一整个 bank。
        /// @note 本函数的实现方式是轮询标志位，不会使用中断和信号量。
        /// @note stm32h743 有 2 个 bank。典型用法是：bank1 用来存放程序，bank2 用来存放数据。
        /// @warning bank_index 为 0 表示 bank1，索引为 1 表示 bank2。
        ///
        /// @param bank_index
        void EraseBank_NoIT(int32_t bank_index);

        /// @brief 擦除指定 bank 的指定扇区。
        /// @param bank_index
        /// @param sector_index 要擦除的扇区索引。
        void EraseSector(int32_t bank_index, int32_t sector_index);
        using bsp::IFlash::EraseSector;

        /// @brief 擦除指定 bank 的指定扇区。
        /// @note 本函数的实现方式是轮询标志位，不会使用中断和信号量。
        ///
        /// @param bank_index
        /// @param sector_index 要擦除的扇区索引。
        void EraseSector_NoIT(int32_t bank_index, int32_t sector_index);
#pragma endregion

        /// @brief 从 flash 中读取数据，写入 buffer。
        /// @param bank_index
        /// @param addr flash 中的数据相对于此 bank 的地址。
        /// @param buffer 要将 flash 中的数据写入此缓冲区。
        /// @param count 要读取多少个字节的数据。
        void Read(int32_t bank_index, size_t addr, uint8_t *buffer, int32_t count) override;

#pragma region 编程
        /// @brief 编程
        ///
        /// @param bank_index
        ///
        /// @param addr 要写入的数据相对于此 bank 的起始地址的地址。
        /// @note 此地址必须能被 MinProgrammingUnit 整除。
        ///
        /// @param buffer 要写入到 flash 的数据所在的缓冲区。大小为 MinProgrammingUnit。
        /// @note 底层在编程时，会读取并且只会读取 MinProgrammingUnit 个字节。
        /// @warning buffer 的字节数必须 >= MinProgrammingUnit ，否则因为底层无论如何都会读取
        /// MinProgrammingUnit 个字节，所以将发生内存访问越界。
        /// @note 不同平台对 buffer 有对齐要求。例如 stm32 的 HAL 要求 buffer 要 4 字节
        /// 对齐。这里使用 uint8_t const * ，接口的实现者自己计算 buffer 能否被对齐字节数整除，
        /// 不能整除抛出异常。
        /// @note 对于 4 字节对齐的情况，调用者可以创建 uint32_t 数组，然后
        /// 将 uint32_t const * 强制转换为 uint8_t const * 作为 buffer 传进来。
        void Program(int32_t bank_index, size_t addr, uint8_t const *buffer) override;

        /// @brief 编程
        /// @note 本函数的实现方式是轮询标志位，不会使用中断和信号量。
        ///
        /// @param bank_index
        ///
        /// @param addr 要写入的数据相对于此 bank 的起始地址的地址。
        /// @note 此地址必须能被 MinProgrammingUnit 整除。
        ///
        /// @param buffer 要写入到 flash 的数据所在的缓冲区。大小为 MinProgrammingUnit。
        /// @note 底层在编程时，会读取并且只会读取 MinProgrammingUnit 个字节。
        /// @warning buffer 的字节数必须 >= MinProgrammingUnit ，否则因为底层无论如何都会读取
        /// MinProgrammingUnit 个字节，所以将发生内存访问越界。
        /// @note 不同平台对 buffer 有对齐要求。例如 stm32 的 HAL 要求 buffer 要 4 字节
        /// 对齐。这里使用 uint8_t const * ，接口的实现者自己计算 buffer 能否被对齐字节数整除，
        /// 不能整除抛出异常。
        /// @note 对于 4 字节对齐的情况，调用者可以创建 uint32_t 数组，然后
        /// 将 uint32_t const * 强制转换为 uint8_t const * 作为 buffer 传进来。
        void Program_NoIT(int32_t bank_index, size_t addr, uint8_t const *buffer);
#pragma endregion
    };
} // namespace bsp
