#pragma once
#ifndef TKP_TOOLS_GBADDR_H
#define TKP_TOOLS_GBADDR_H
#include "../include/disassembly_instr.h"
#include "gb_breakpoint.h"
#include <cstdint>
using RegisterType = uint8_t;
using BigRegisterType = uint16_t;
using DisInstr = TKPEmu::Tools::DisInstr;
using GBBPArguments = TKPEmu::Gameboy::Utils::GBBPArguments;
constexpr size_t LogTypeSize = 17;
enum class LogType {
    InstrName, InstrNum,
    A, B, C, D, 
    E, F, H, L,
    PC, SP, LY,
    IF, IE, IME, HALT
};
enum LCDCFlag {
    BG_ENABLE = 1 << 0,
    OBJ_ENABLE = 1 << 1,
    OBJ_SIZE = 1 << 2,
    BG_TILEMAP = 1 << 3,
    BG_TILES = 1 << 4,
    WND_ENABLE = 1 << 5,
    WND_TILEMAP = 1 << 6,
    LCD_ENABLE = 1 << 7
};
enum STATFlag {
    MODE = 0b11,
    COINCIDENCE = 1 << 2,
    MODE0_INTER = 1 << 3,
    MODE1_INTER = 1 << 4,
    MODE2_INTER = 1 << 5,
    COINC_INTER = 1 << 6
};
enum IFInterrupt {
    VBLANK = 1 << 0,
    LCDSTAT = 1 << 1,
    TIMER = 1 << 2,
    SERIAL = 1 << 3,
    JOYPAD = 1 << 4
};
// Used in the map below, to compare rom hashes with expected results after
// a hardcoded number of clocks
using Hash = std::string;
struct ExpectedResult {
    unsigned long long Clocks;
    // Represents the hash of the screenshot taken after Clocks
    Hash ExpectedHash;
};
// This map helps with quality assurance, we can check multiple test roms
// at once and compare their finished hashes with these known good results
const static std::unordered_map<Hash, ExpectedResult> PassedTestMap {
    // cpu_instrs
    { "662f04537286d13ee55a6df9de4dce24", { 0, "error-hash" } },
    // bits_bank1
    { "31d2ad77d05566b1dcb92d7ff7232767", { 0, "f0553923b341e4e57f091f1ce8cc8cf0" } },
    // bits_bank2
    { "d7c8eb26b8c276f0d13e2a272a17c308", { 0, "f0553923b341e4e57f091f1ce8cc8cf0" } },
    // bits_mode
    { "d807288f6cfe34225989dd0d9293043e", { 0, "f0553923b341e4e57f091f1ce8cc8cf0" } },
    // if_ie_registers
    { "cd20692d8453b777bb2918bef8431117", { 0, "f2ead6ee6dc0a2f45504a43ad6536ba6" } },
    // div_write
    { "c57eada752f746347951f79c828391b9", { 0, "f0553923b341e4e57f091f1ce8cc8cf0" } },
};
constexpr static std::array<LogType, LogTypeSize> LogTypeMap {
    LogType::InstrName, LogType::InstrNum,
    LogType::A, LogType::B, LogType::C, LogType::D,
    LogType::E, LogType::F, LogType::H, LogType::L,
    LogType::PC, LogType::SP, LogType::LY, LogType::IF,
    LogType::IE, LogType::IME, LogType::HALT
};
const static std::array<std::string, LogTypeSize> LogTypeNames {
    "Instr. ", "Opcode ",
    "A      ", "B      ", "C      ", "D      ",
    "E      ", "F      ", "H      ", "L      ",
    "PC     ", "SP     ", "LY     ", "IF     ",
    "IE     ", "IME    ", "HALT   ",
};
constexpr std::array<uint8_t, 0x100> InstrTimes = {
    0, 2, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0,
    0, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0,
    1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0,
    1, 2, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 2, 2, 2, 0, 1, 0, 0, 0, 2, 1, 2, 2, 2, 0,
    0, 0, 2, 0, 2, 0, 1, 0, 0, 0, 2, 0, 2, 0, 1, 1,
    0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 2,
    1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 1, 0
};
constexpr auto cl_white = 0;
constexpr auto cl_lgray = 1;
constexpr auto cl_dgray = 2;
constexpr auto cl_black = 3;
constexpr auto addr_joy = 0xFF00;
// Serial registers
constexpr auto addr_std = 0xFF01;
constexpr auto addr_stc = 0xFF02;
// Timer registers
constexpr auto addr_div = 0xFF04;
constexpr auto addr_tim = 0xFF05;
constexpr auto addr_tma = 0xFF06;
constexpr auto addr_tac = 0xFF07;
// Interrupt flag
constexpr auto addr_ifl = 0xFF0F;
// Sound registers
constexpr auto addr_s1s = 0xFF10;
constexpr auto addr_s3e = 0xFF1A;
constexpr auto addr_s3o = 0xFF1C;
constexpr auto addr_s4l = 0xFF20;
constexpr auto addr_s4c = 0xFF23;
constexpr auto addr_snd = 0xFF26;
// PPU & OAM related registers
constexpr auto addr_lcd = 0xFF40;
constexpr auto addr_sta = 0xFF41;
constexpr auto addr_lly = 0xFF44;
constexpr auto addr_dma = 0xFF46;
constexpr auto addr_bgp = 0xFF47;
constexpr auto addr_ob0 = 0xFF48;
constexpr auto addr_ob1 = 0xFF49;

constexpr auto addr_if = 0xFF0F;
constexpr auto addr_ie = 0xFFFF;

// CPU flag masks and shifts
constexpr uint8_t FLAG_ZERO_MASK = 0b10000000;
constexpr uint8_t FLAG_NEG_MASK = 0b01000000;
constexpr uint8_t FLAG_HCARRY_MASK = 0b00100000;
constexpr uint8_t FLAG_CARRY_MASK = 0b00010000;
constexpr uint8_t FLAG_EMPTY_MASK = 0b00000000;
constexpr uint8_t FLAG_ZERO_SHIFT = 7;
constexpr uint8_t FLAG_NEG_SHIFT = 6;
constexpr uint8_t FLAG_HCARRY_SHIFT = 5;
constexpr uint8_t FLAG_CARRY_SHIFT = 4;
#endif