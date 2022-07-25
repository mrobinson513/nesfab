#include "mapper.hpp"

#include <stdexcept>
#include <cstring>

#include "builtin.hpp"

mapper_t mapper_t::nrom(mapper_mirroring_t mirroring)
{
    return 
    {
        .type = MAPPER_NROM,
        .mirroring = mirroring,
        .num_32k_banks = 1,
        .num_8k_chr_rom = 1,
        .num_8k_chr_ram = 0,
    };
}

mapper_t mapper_t::bnrom(mapper_mirroring_t mirroring, unsigned banks_32k)
{
    assert(mirroring != MIRROR_4);
    return 
    {
        .type = MAPPER_BNROM,
        .mirroring = mirroring,
        .num_32k_banks = banks_32k,
        .num_8k_chr_rom = 0,
        .num_8k_chr_ram = 1,
    };
}

mapper_t mapper_t::gtrom(unsigned banks_32k)
{
    return 
    {
        .type = MAPPER_GTROM,
        .mirroring = MIRROR_4,
        .num_32k_banks = banks_32k,
        .num_8k_chr_rom = 0,
        .num_8k_chr_ram = 2,
    };
}

void write_ines_header(std::uint8_t* at, mapper_t const& mapper)
{
    // https://www.nesdev.org/wiki/NES_2.0

    // 0-3
    char const magic_header[4] = { 0x4E, 0x45, 0x53, 0x1A };
    std::memcpy(at, magic_header, 4);

    // 4
    at[4] = std::uint8_t(mapper.num_16k_banks()); // Banks in 16k units, low byte.

    // 5
    at[5] = std::uint8_t(mapper.num_8k_chr_rom); // Banks in 16k units.

    // 6
    std::uint8_t flags6 = 0;
    flags6 |= unsigned(mapper.type) << 4;
    switch(mapper.mirroring)
    {
    case MIRROR_H: break;
    case MIRROR_V: flags6 |= 1 << 0; break;
    case MIRROR_4: flags6 |= 1 << 3; break;
    }
    at[6] = flags6;

    // 7
    std::uint8_t flags7 = 0b00001000; // NES 2.0 format
    flags7 |= unsigned(mapper.type) & 0b11110000;
    at[7] = flags7;

    // 8
    std::uint8_t flags8 = 0;
    flags8 |= (unsigned(mapper.type) >> 8) & 0b1111;
    at[8] = flags8;

    // 9
    if(((mapper.num_16k_banks()) >> 8) > 0b1111)
        throw std::runtime_error("Too many ROM banks.");

    if(((mapper.num_8k_chr_rom) >> 8) > 0b1111)
        throw std::runtime_error("Too many CHR RAM banks.");

    std::uint8_t hi = 0;
    hi |= ((mapper.num_16k_banks()) >> 8) & 0b1111;
    hi |= ((mapper.num_8k_chr_rom) >> 4) & 0b11110000;
    at[9] = hi;

    // 10
    at[10] = 0;

    // 11
    unsigned const chr_ram_chunks = mapper.num_8k_chr_ram * 0x2000 / 64;
    if(chr_ram_chunks && builtin::popcount(chr_ram_chunks) != 1)
        throw std::runtime_error("Invalid CHR RAM size.");
    unsigned const chr_shift = chr_ram_chunks ? builtin::rclz(chr_ram_chunks)-1 : 0;
    assert(!chr_ram_chunks || 64 << chr_shift == mapper.num_8k_chr_ram * 0x2000);

    if(chr_shift > 0b1111)
        throw std::runtime_error("CHR RAM is too large.");

    at[11] = chr_shift & 0b1111;

    // 12
    at[12] = 0;

    // 13
    at[13] = 0;

    // 14
    at[14] = 0;

    // 15
    at[15] = 0;
}
