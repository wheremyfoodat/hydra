#pragma once

#include "nes_apu.hxx"
#include "nes_cpu.hxx"
#include "nes_cpubus.hxx"
#include "nes_ppu.hxx"
#include <emulator.hxx>

namespace hydra::NES
{
    class NES_TKPWrapper : public Emulator
    {
        TKP_EMULATOR(NES_TKPWrapper);

    private:
        PPU ppu_{};
        APU apu_{};
        CPUBus cpubus_{ppu_, apu_};
        CPU cpu_{cpubus_, Paused};

        std::chrono::time_point<std::chrono::high_resolution_clock> start_frame_time_{};
    };
} // namespace hydra::NES
