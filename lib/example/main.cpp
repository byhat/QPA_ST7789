/**
 * @file main.cpp
 * @brief Демонстрация драйвера ST7789: сборка слоёв и заливка цветами.
 *
 * Слои собираются здесь: SpidevBus и WiringGpioLine (доступ к железу)
 * передаются в St7789Driver по ссылке. Кадры формируются вектором
 * пикселей RGB565 и отдаются драйверу через blit() - тот же контракт,
 * что у QPA-плагина.
 */

#include "SpidevBus.hpp"
#include "WiringGpioLine.hpp"
#include "St7789Driver.hpp"

#include <cstdint>
#include <iostream>
#include <string>
#include <system_error>
#include <thread>
#include <chrono>
#include <vector>

namespace {

/// Путь к SPI-устройству шины дисплея (SPI1, CS1).
constexpr const char *kSpiDevice = "/dev/spidev1.1";

/// Физические пины разъёма Orange Pi Zero 3 (wiringPiSetupPhys).
constexpr int kChipSelectPin = 26;  ///< PC10
constexpr int kDataCommandPin = 22; ///< PC7
constexpr int kResetPin = 18;       ///< PC14

/// Параметры панели 1.14" 135x240 и скоростей шины.
constexpr St7789Driver::Config kPanelConfig = {
    135, 240, 1'000'000, 32'000'000, 40, 52, true
};

/// Цвета RGB565 для демонстрационных кадров.
constexpr std::uint16_t kColorRed = 0xF800;
constexpr std::uint16_t kColorGreen = 0x07E0;
constexpr std::uint16_t kColorBlue = 0x001F;

/**
 * @brief Печатает пойманное исключение и возвращает код ошибки.
 * @param error перехваченное исключение
 * @return код процесса 1
 */
int reportError(const std::exception &error)
{
    std::cerr << "gg: " << error.what() << std::endl;
    return 1;
}

} // namespace

int main()
{
    try {
        SpidevBus spiBus(kSpiDevice);
        WiringGpioLine chipSelect(kChipSelectPin);
        WiringGpioLine dataCommand(kDataCommandPin);
        WiringGpioLine reset(kResetPin);

        St7789Driver driver(spiBus, chipSelect, dataCommand, reset, kPanelConfig);
        driver.initialize();

        // частичные blit: квадраты в разных местах, без перерисовки кадра;
        // фон тёмный, поверх - цветные квадраты по очереди
        const std::size_t frameStrideBytes =
            static_cast<std::size_t>(kPanelConfig.width) * sizeof(std::uint16_t);
        std::vector<std::uint16_t> frame(
            static_cast<std::size_t>(kPanelConfig.width) * kPanelConfig.height, 0);
        driver.blit(0, 0, kPanelConfig.width, kPanelConfig.height,
                    frame.data(), frameStrideBytes);
        std::cout << "gg: background" << std::endl;

        struct Square {
            int x;
            int y;
            int size;
            std::uint16_t color;
            const char *name;
        };
        const Square squares[] = {
            {10, 10, 40, kColorRed, "red 10,10"},
            {75, 40, 45, kColorGreen, "green 75,40"},
            {25, 120, 50, kColorBlue, "blue 25,120"},
            {75, 180, 40, 0xFFFF, "white 75,180"},
        };
        for (const Square &square : squares) {
            std::vector<std::uint16_t> patch(
                static_cast<std::size_t>(square.size) * square.size, square.color);
            driver.blit(square.x, square.y, square.size, square.size,
                        patch.data(),
                        static_cast<std::size_t>(square.size) * sizeof(std::uint16_t));
            std::cout << "gg: square " << square.name << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        std::cout << "gg: finished" << std::endl;
    } catch (const std::system_error &error) {
        return reportError(error);
    } catch (const std::exception &error) {
        return reportError(error);
    }
    return 0;
}
