#include <algorithm>
#include <bit>
#include <bitset>
#include <cassert>
#include <compatibility.hxx>
#include <fstream>
#include <functional>
#include <iostream>
#include <log.hxx>
#include <n64/core/n64_addresses.hxx>
#include <n64/core/n64_rdp.hxx>
#include <n64/core/n64_rdp_commands.hxx>
#include <sstream>
#include <str_hash.hxx>

// For debugging purposes
constexpr bool slow_assertions = true;

hydra_inline static uint32_t irand(uint32_t* state)
{
    *state = *state * 0x343fd + 0x269ec3;
    return ((*state >> 16) & 0x7fff);
}

hydra_inline static uint32_t rgba16_to_rgba32(uint16_t color)
{
    uint8_t r16 = (color >> 11) & 0x1F;
    uint8_t g16 = (color >> 6) & 0x1F;
    uint8_t b16 = (color >> 1) & 0x1F;
    uint8_t r = (r16 << 3) | (r16 >> 2);
    uint8_t g = (g16 << 3) | (g16 >> 2);
    uint8_t b = (b16 << 3) | (b16 >> 2);
    uint8_t a = (color & 0x1) ? 0xFF : 0x00;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

hydra_inline static uint16_t rgba32_to_rgba16(uint32_t color)
{
    uint8_t r = (color >> 3) & 0x1F;
    uint8_t g = (color >> 11) & 0x1F;
    uint8_t b = (color >> 19) & 0x1F;
    uint8_t a = (color >> 24) & 0x1;
    return (r << 11) | (g << 6) | (b << 1) | a;
}

static std::pair<int32_t, int32_t> perspective_correction(int32_t s, int32_t t, int32_t w)
{
    if ((w >> 15) == 0)
    {
        Logger::WarnOnce("Division by zero in perspective correction");
        return {0, 0};
    }
    return {(s / (w >> 15)) >> 5, (t / (w >> 15)) >> 5};
}

static std::pair<int32_t, int32_t> no_perspective_correction(int32_t s, int32_t t, int32_t w)
{
    return {(int16_t)s >> 16, (int16_t)t >> 16};
}

namespace hydra::N64
{
    constexpr inline std::string_view get_rdp_command_name(RDPCommandType type)
    {
        switch (type)
        {
#define X(name, opcode, length) \
    case RDPCommandType::name:  \
        return #name;
            RDP_COMMANDS
#undef X
            default:
                return "Unknown";
        }
    }

    constexpr inline int get_rdp_command_length(RDPCommandType type)
    {
        switch (type)
        {
#define X(name, opcodes, length) \
    case RDPCommandType::name:   \
        return length;
            RDP_COMMANDS
#undef X
            default:
                Logger::Warn("Unknown command length for command: {} ({:02x})",
                             get_rdp_command_name(type), static_cast<int>(type));
                return 1;
        }
    }

    RDP::RDP()
    {
        rdram_9th_bit_.resize(0x800000);
        init_depth_luts();
    }

    void RDP::InstallBuses(uint8_t* rdram_ptr, uint8_t* spmem_ptr)
    {
        rdram_ptr_ = rdram_ptr;
        spmem_ptr_ = spmem_ptr;
    }

    uint32_t RDP::ReadWord(uint32_t addr)
    {
        switch (addr)
        {
            case DP_START:
                return start_address_;
            case DP_END:
                return end_address_;
            case DP_STATUS:
                return status_.full;
            case DP_CLOCK:
                return 0; // ???
            case DP_BUSY:
                return 0; // ???
            default:
            {
                Logger::WarnOnce("RDP: Unhandled read from {:08X}", addr);
                return 0;
            }
        }
    }

    void RDP::WriteWord(uint32_t addr, uint32_t data)
    {
        switch (addr)
        {
            case DP_START:
            {
                if (!status_.start_pending)
                {
                    start_address_ = data & 0xFFFFF8;
                }
                status_.start_pending = 1;
                break;
            }
            case DP_END:
            {
                if (status_.start_pending)
                {
                    // New transfer
                    status_.start_pending = 0;
                    // TODO: test if we can safely uncomment or if is correct
                    // status_.dma_busy = 1;
                    current_address_ = start_address_;
                }
                end_address_ = data & 0xFFFFF8;
                status_.pipe_busy = 1;
                status_.start_gclk = 1;
                process_commands();
                status_.ready = 1;
                break;
            }
            case DP_STATUS:
            {
                RDPStatusWrite write;
                write.full = data;
#define flag(x)                                 \
    if (write.clear_##x && !write.set_##x)      \
        status_.x = 0;                          \
    else if (write.set_##x && !write.clear_##x) \
        status_.x = 1;
                flag(dma_source_dmem);
                flag(freeze);
                flag(flush);
                if (write.clear_tmem_busy)
                {
                    status_.tmem_busy = 0;
                }
                if (write.clear_pipe_busy)
                {
                    status_.pipe_busy = 0;
                }
                if (write.clear_buffer_busy)
                {
                    status_.cmd_busy = 0;
                }
#undef flag
                break;
            }
        }
    }

    void RDP::Reset()
    {
        seed_ = 3;
        status_.ready = 1;
        color_sub_a_[0] = color_sub_a_[1] = &color_one_;
        color_sub_b_[0] = color_sub_b_[1] = &color_zero_;
        color_multiplier_[0] = color_multiplier_[1] = &color_one_;
        color_adder_[0] = color_adder_[1] = &color_zero_;
        alpha_sub_a_[0] = alpha_sub_a_[1] = &color_zero_;
        alpha_sub_b_[0] = alpha_sub_b_[1] = &color_zero_;
        alpha_multiplier_[0] = alpha_multiplier_[1] = &color_one_;
        alpha_adder_[0] = alpha_adder_[1] = &color_zero_;
        blender_1a_[0] = blender_1a_[1] = 0;
        blender_1b_[0] = blender_1b_[1] = 0;
        blender_2a_[0] = blender_2a_[1] = 0;
        blender_2b_[0] = blender_2b_[1] = 0;
        texel_color_[0] = texel_color_[1] = 0xFFFFFFFF;
        texel_alpha_[0] = texel_alpha_[1] = 0xFFFFFFFF;
        cycle_type_ = CycleType::Cycle1;
        perspective_correction_func_ = &no_perspective_correction;
    }

    void RDP::SendCommand(const std::vector<uint64_t>& data)
    {
        execute_command(data);
    }

    void RDP::process_commands()
    {
        uint32_t current = current_address_ & 0xFFFFF8;
        uint32_t end = end_address_ & 0xFFFFF8;

        status_.freeze = 1;
        while (current < end)
        {
            uintptr_t address = status_.dma_source_dmem ? reinterpret_cast<uintptr_t>(spmem_ptr_)
                                                        : reinterpret_cast<uintptr_t>(rdram_ptr_);
            uint64_t data = hydra::bswap64(*reinterpret_cast<uint64_t*>(address + current));
            uint8_t command_type = (data >> 56) & 0b111111;

            if (command_type >= 8)
            {
                int length = get_rdp_command_length(static_cast<RDPCommandType>(command_type));
                std::vector<uint64_t> command;
                command.resize(length);
                for (int i = 0; i < length; i++)
                {
                    command[i] =
                        hydra::bswap64(*reinterpret_cast<uint64_t*>(address + current + (i * 8)));
                }
                execute_command(command);
                // Logger::Info("RDP: Command {} ({:02x})",
                // get_rdp_command_name(static_cast<RDPCommandType>(command_type)),
                // static_cast<int>(command_type));
                current += length * 8;
            }
            else
            {
                current += 8;
            }
        }

        current_address_ = end_address_;
        status_.freeze = 0;
    }

    void RDP::execute_command(const std::vector<uint64_t>& data)
    {
        RDPCommandType id = static_cast<RDPCommandType>((data[0] >> 56) & 0b111111);
        // Logger::Info("RDP: {}", get_rdp_command_name(id));
        switch (id)
        {
            case RDPCommandType::SyncFull:
            {
                Logger::Debug("Raising DP interrupt");
                mi_interrupt_->DP = true;
                status_.dma_busy = false;
                status_.pipe_busy = false;
                status_.start_gclk = false;
                break;
            }
            case RDPCommandType::SetColorImage:
            {
                SetColorImageCommand color_format;
                color_format.full = data[0];
                framebuffer_dram_address_ = color_format.dram_address;
                framebuffer_width_ = color_format.width + 1;
                framebuffer_format_ = color_format.format;
                // 0 = 4bpp, 1 = 8bpp, 2 = 16bpp, 3 = 32bpp
                framebuffer_pixel_size_ = 4 * (1 << color_format.size);
                break;
            }
            case RDPCommandType::Triangle:
            case RDPCommandType::TriangleDepth:
            case RDPCommandType::TriangleTexture:
            case RDPCommandType::TriangleTextureDepth:
            case RDPCommandType::TriangleShade:
            case RDPCommandType::TriangleShadeDepth:
            case RDPCommandType::TriangleShadeTexture:
            case RDPCommandType::TriangleShadeTextureDepth:
            {
                // printf("custom_triangle({");
                // for (size_t i = 0; i < data.size(); i++)
                // {
                //     printf("0x%016lx, ", data[i]);
                // }
                // printf("});\n");
                uint8_t id8 = static_cast<uint8_t>(id);
                bool depth = id8 & 0b1;
                bool texture = id8 & 0b10;
                bool shade = id8 & 0b100;
                EdgewalkerInput input = triangle_get_edgewalker_input(data, shade, texture, depth);
                Primitive primitive = edgewalker(input);

                if (slow_assertions)
                {
                    check_primitive(primitive, input, data);
                }

                render_primitive(primitive);
                break;
            }
            case RDPCommandType::Rectangle:
            {
                EdgewalkerInput input = rectangle_get_edgewalker_input<false, false>(data);
                Primitive primitive = edgewalker(input);

                if (slow_assertions)
                {
                    check_primitive(primitive, input, data);
                }

                render_primitive(primitive);
                break;
            }
            case RDPCommandType::TextureRectangle:
            {
                EdgewalkerInput input = rectangle_get_edgewalker_input<true, false>(data);
                Primitive primitive = edgewalker(input);

                if (slow_assertions)
                {
                    check_primitive(primitive, input, data);
                }

                render_primitive(primitive);
                break;
            }
            case RDPCommandType::TextureRectangleFlip:
            {
                EdgewalkerInput input = rectangle_get_edgewalker_input<true, true>(data);
                Primitive primitive = edgewalker(input);
                render_primitive(primitive);
                break;
            }
            case RDPCommandType::SetFillColor:
            {
                fill_color_32_ = data[0] & 0xFFFFFFFF;
                fill_color_16_0_ = data[0] & 0xFFFF;
                fill_color_16_1_ = (data[0] >> 16);
                fill_color_32_ = hydra::bswap32(fill_color_32_);
                break;
            }
            case RDPCommandType::LoadTile:
            {
                // Loads a tile (part of the bigger texture set by SetTextureImage) into TMEM
                LoadTileCommand command;
                command.full = data[0];

                load_tile(command);
                break;
            }
            case RDPCommandType::LoadBlock:
            {
                LoadBlockCommand command;
                command.full = data[0];
                TileDescriptor& tile = tiles_[command.tile];

                int sl = command.SL;
                int sh = command.SH;

                switch (texture_pixel_size_latch_)
                {
                    case 16:
                    {
                        sh *= sizeof(uint16_t);
                        sl *= sizeof(uint16_t);
                        for (int i = sl; i < sh; i += 8)
                        {
                            uint64_t src = *reinterpret_cast<uint64_t*>(
                                &rdram_ptr_[texture_dram_address_latch_ + i]);
                            uint8_t* dst =
                                reinterpret_cast<uint8_t*>(&tmem_[tile.tmem_address + i]);
                            memcpy(dst, &src, 8);
                        }
                        break;
                    }
                    case 32:
                    {
                        // for (int i = sl; i < sh; i += 8)
                        // {
                        //     uint64_t src = *reinterpret_cast<uint64_t*>(
                        //         &rdram_ptr_[texture_dram_address_latch_ + i]);
                        //     // Write 8 bytes of texture data to TMEM, split across high and low
                        //     banks uint8_t* dst =
                        //         reinterpret_cast<uint8_t*>(&tmem_[tile.tmem_address + i]);
                        //     for (int j = 0; j < 4; j += 2)
                        //     {
                        //         dst[j + 0] = src >> (56 - j * 16);
                        //         dst[j + 1] = src >> (48 - j * 16);
                        //         dst[j + 2] = src >> (40 - j * 16);
                        //         dst[j + 3] = src >> (32 - j * 16);
                        //     }
                        // }
                        break;
                    }
                    default:
                    {
                        Logger::WarnOnce("Using unimplemented size for LoadBlock: {}",
                                         texture_pixel_size_latch_);
                        break;
                    }
                }
                break;
            }
            case RDPCommandType::SetTile:
            {
                // The gsDPSetTile is used to indicate where in Tmem you want to place the image,
                // how wide each line is, and the format and size of the texture.
                SetTileCommand command;
                command.full = data[0];
                TileDescriptor& tile = tiles_[command.Tile];
                tile.tmem_address = command.TMemAddress;
                tile.format = static_cast<Format>(command.format);
                tile.size = 4 * (1 << command.size);
                tile.line_width = command.Line * 8; // in 64bit / 8 byte words
                // This number is used as the MS 4b of an 8b index.
                tile.palette_index = command.Palette << 4;
                tile.clamp_s = command.cs;
                tile.clamp_t = command.ct;
                tile.mirror_s = command.ms;
                tile.mirror_t = command.mt;
                if (command.MaskS == 0)
                    tile.mask_s = -1;
                else
                    tile.mask_s = (1 << command.MaskS) - 1;
                if (command.MaskT == 0)
                    tile.mask_t = -1;
                else
                    tile.mask_t = (1 << command.MaskT) - 1;
                break;
            }
            case RDPCommandType::SetTileSize:
            {
                SetTileSizeCommand command;
                command.full = data[0];
                TileDescriptor& tile = tiles_[command.Tile];
                tile.sl = command.SL;
                tile.tl = command.TL;
                tile.sh = command.SH;
                tile.th = command.TH;
                break;
            }
            case RDPCommandType::SetTextureImage:
            {
                // The gsDPSetTextureImage command sets a pointer to the location of the image.
                SetTextureImageCommand command;
                command.full = data[0];
                texture_dram_address_latch_ = command.DRAMAddress;
                texture_width_latch_ = command.width + 1;
                texture_format_latch_ = static_cast<Format>(command.format);
                texture_pixel_size_latch_ = (1 << command.size) * 4;
                break;
            }
            case RDPCommandType::SetOtherModes:
            {
                SetOtherModesCommand command;
                command.full = data[0];
                cycle_type_ = static_cast<CycleType>(command.cycle_type);
                z_compare_en_ = command.z_compare_en;
                z_update_en_ = command.z_update_en;
                z_source_sel_ = command.z_source_sel;
                z_mode_ = command.z_mode;

                blender_1a_[0] = command.b_m1a_0;
                blender_1b_[0] = command.b_m1b_0;
                blender_2a_[0] = command.b_m2a_0;
                blender_2b_[0] = command.b_m2b_0;

                blender_1a_[1] = command.b_m1a_1;
                blender_1b_[1] = command.b_m1b_1;
                blender_2a_[1] = command.b_m2a_1;
                blender_2b_[1] = command.b_m2b_1;

                image_read_en_ = command.image_read_en;
                alpha_compare_en_ = command.alpha_compare_en;
                if (command.persp_tex_en)
                {
                    perspective_correction_func_ = &perspective_correction;
                }
                else
                {
                    perspective_correction_func_ = &no_perspective_correction;
                }
                break;
            }
            case RDPCommandType::SetPrimDepth:
            {
                primitive_depth_ = data[0] & (0x7FFF << 16);
                primitive_depth_delta_ = data[0] & 0xFFFF;
                break;
            }
            case RDPCommandType::SetZImage:
            {
                zbuffer_dram_address_ = data[0] & 0x1FFFFFF;
                break;
            }
            case RDPCommandType::SetEnvironmentColor:
            {
                environment_color_ = data[0] & 0xFFFFFFFF;
                environment_color_ = hydra::bswap32(environment_color_);

                uint8_t alpha = environment_color_ >> 24;
                environment_alpha_ = (alpha << 24) | (alpha << 16) | (alpha << 8) | alpha;
                break;
            }
            case RDPCommandType::SetBlendColor:
            {
                blend_color_ = data[0] & 0xFFFFFFFF;
                blend_color_ = hydra::bswap32(blend_color_);
                break;
            }
            case RDPCommandType::SetPrimitiveColor:
            {
                primitive_color_ = data[0] & 0xFFFFFFFF;
                primitive_color_ = hydra::bswap32(primitive_color_);

                uint8_t alpha = primitive_color_ >> 24;
                primitive_alpha_ = (alpha << 24) | (alpha << 16) | (alpha << 8) | alpha;
                break;
            }
            case RDPCommandType::SetScissor:
            {
                SetScissorCommand command;
                command.full = data[0];
                scissor_xh_ = command.XH;
                scissor_yh_ = command.YH;
                scissor_xl_ = command.XL;
                scissor_yl_ = command.YL;
                break;
            }
            case RDPCommandType::SetFogColor:
            {
                fog_color_ = data[0] & 0xFFFFFFFF;
                fog_color_ = hydra::bswap32(fog_color_);

                uint8_t alpha = fog_color_ >> 24;
                fog_alpha_ = (alpha << 24) | (alpha << 16) | (alpha << 8) | alpha;
                break;
            }
            case RDPCommandType::SetCombineMode:
            {
                SetCombineModeCommand command;
                command.full = data[0];

                color_sub_a_[0] = color_get_sub_a(command.sub_A_RGB_0);
                color_sub_b_[0] = color_get_sub_b(command.sub_B_RGB_0);
                color_multiplier_[0] = color_get_mul(command.mul_RGB_0);
                color_adder_[0] = color_get_add(command.add_RGB_0);

                color_sub_a_[1] = color_get_sub_a(command.sub_A_RGB_1);
                color_sub_b_[1] = color_get_sub_b(command.sub_B_RGB_1);
                color_multiplier_[1] = color_get_mul(command.mul_RGB_1);
                color_adder_[1] = color_get_add(command.add_RGB_1);

                alpha_sub_a_[0] = alpha_get_sub_add(command.sub_A_Alpha_0);
                alpha_sub_b_[0] = alpha_get_sub_add(command.sub_B_Alpha_0);
                alpha_multiplier_[0] = alpha_get_mul(command.mul_Alpha_0);
                alpha_adder_[0] = alpha_get_sub_add(command.add_Alpha_0);

                alpha_sub_a_[1] = alpha_get_sub_add(command.sub_A_Alpha_1);
                alpha_sub_b_[1] = alpha_get_sub_add(command.sub_B_Alpha_1);
                alpha_multiplier_[1] = alpha_get_mul(command.mul_Alpha_1);
                alpha_adder_[1] = alpha_get_sub_add(command.add_Alpha_1);
                break;
            }
            case RDPCommandType::SetKeyR:
            case RDPCommandType::SetKeyGB:
            case RDPCommandType::SetConvert:
            {
                Logger::WarnOnce("Unhandled YUV related command: {} ({:02x})",
                                 get_rdp_command_name(id), static_cast<int>(id));
                break;
            }
            default:
                Logger::Debug("Unhandled command: {} ({:02x})", get_rdp_command_name(id),
                              static_cast<int>(id));
                break;
        }
    }

    uint32_t* RDP::color_get_sub_a(uint8_t sub_a)
    {
        switch (sub_a & 0b1111)
        {
            case 0:
                return &combined_color_;
            case 1:
                return &texel_color_[0];
            case 2:
                return &texel_color_[1];
            case 3:
                return &primitive_color_;
            case 4:
                return &shade_color_;
            case 5:
                return &environment_color_;
            case 6:
                return &color_one_;
            case 7:
                return &noise_color_;
            default:
                return &color_zero_;
        }
    }

    uint32_t* RDP::color_get_sub_b(uint8_t sub_b)
    {
        switch (sub_b & 0b1111)
        {
            case 0:
                return &combined_color_;
            case 1:
                return &texel_color_[0];
            case 2:
                return &texel_color_[1];
            case 3:
                return &primitive_color_;
            case 4:
                return &shade_color_;
            case 5:
                return &environment_color_;
            // TODO: Key center??
            case 6:
                return &color_zero_;
            // TODO: Convert K4??
            case 7:
                return &color_zero_;
            default:
                return &color_zero_;
        }
    }

    uint32_t* RDP::color_get_mul(uint8_t mul)
    {
        switch (mul & 0b11111)
        {
            case 0:
                return &combined_color_;
            case 1:
                return &texel_color_[0];
            case 2:
                return &texel_color_[1];
            case 3:
                return &primitive_color_;
            case 4:
                return &shade_color_;
            case 5:
                return &environment_color_;
            case 7:
                return &combined_alpha_;
            case 8:
                return &texel_alpha_[0];
            case 9:
                return &texel_alpha_[1];
            case 10:
                return &primitive_alpha_;
            case 11:
                return &shade_alpha_;
            case 12:
                return &environment_alpha_;
            // TODO: rest of the colors
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21:
            case 22:
            case 23:
            case 24:
            case 25:
            case 26:
            case 27:
            case 28:
            case 29:
            case 30:
            case 31:
                return &color_zero_;
            default:
                Logger::WarnOnce("Unhandled mul: {}", mul);
                return &color_zero_;
        }
    }

    uint32_t* RDP::color_get_add(uint8_t add)
    {
        switch (add & 0b111)
        {
            case 0:
                return &combined_color_;
            case 1:
                return &texel_color_[0];
            case 2:
                return &texel_color_[1];
            case 3:
                return &primitive_color_;
            case 4:
                return &shade_color_;
            case 5:
                return &environment_color_;
            case 6:
                return &color_one_;
            case 7:
                return &color_zero_;
        }
        Logger::Fatal("Unreachable!");
        return nullptr;
    }

    uint32_t* RDP::alpha_get_sub_add(uint8_t sub_a)
    {
        switch (sub_a & 0b111)
        {
            case 0:
                return &combined_alpha_;
            case 1:
                return &texel_alpha_[0];
            case 2:
                return &texel_alpha_[1];
            case 3:
                return &primitive_alpha_;
            case 4:
                return &shade_alpha_;
            case 5:
                return &environment_alpha_;
            case 6:
                return &color_one_;
            default:
                return &color_zero_;
        }
    }

    uint32_t* RDP::alpha_get_mul(uint8_t mul)
    {
        switch (mul & 0b111)
        {
            case 0:
            {
                Logger::WarnOnce("Unhandled alpha mul: LOD fraction", mul);
                return &color_one_;
            }
            case 1:
                return &texel_alpha_[0];
            case 2:
                return &texel_alpha_[1];
            case 3:
                return &primitive_alpha_;
            case 4:
                return &shade_alpha_;
            case 5:
                return &environment_alpha_;
            case 6:
            {
                Logger::WarnOnce("Unhandled alpha mul: Primitive LOD fraction", mul);
                return &color_zero_;
            }
            default:
                return &color_zero_;
        }
    }

    void RDP::draw_pixel(int x, int y)
    {
        uintptr_t address = reinterpret_cast<uintptr_t>(rdram_ptr_) + framebuffer_dram_address_ +
                            (y * framebuffer_width_ + x) * (framebuffer_pixel_size_ >> 3);
        switch (cycle_type_)
        {
            case CycleType::Cycle2:
            {
                color_combiner(0);
                color_combiner(1);
                blender(0);
                // TODO: remove code duplication
                if (framebuffer_pixel_size_ == 16)
                {
                    uint16_t* ptr = reinterpret_cast<uint16_t*>(address);
                    framebuffer_color_ = rgba16_to_rgba32(*ptr);
                    *ptr = rgba32_to_rgba16(blender(1));
                }
                else
                {
                    uint32_t* ptr = reinterpret_cast<uint32_t*>(address);
                    framebuffer_color_ = *ptr;
                    *ptr = blender(1);
                }
                break;
            }
            case CycleType::Cycle1:
            {
                // TODO: there's may be a way to check which cycle we should get the data from
                color_combiner(1);
                if (framebuffer_pixel_size_ == 16)
                {
                    uint16_t* ptr = reinterpret_cast<uint16_t*>(address);
                    framebuffer_color_ = rgba16_to_rgba32(*ptr);
                    *ptr = rgba32_to_rgba16(blender(0));
                }
                else
                {
                    uint32_t* ptr = reinterpret_cast<uint32_t*>(address);
                    framebuffer_color_ = *ptr;
                    *ptr = blender(0);
                }
                break;
            }
            case CycleType::Copy:
            {
                if (alpha_compare_en_ && texel_alpha_[0] == 0)
                {
                    break;
                }

                if (framebuffer_pixel_size_ == 16)
                {
                    uint16_t* ptr = reinterpret_cast<uint16_t*>(address);
                    *ptr = rgba32_to_rgba16(texel_color_[0]);
                }
                else
                {
                    uint32_t* ptr = reinterpret_cast<uint32_t*>(address);
                    *ptr = texel_color_[0];
                }
                break;
            }
            case CycleType::Fill:
            {
                if (framebuffer_pixel_size_ == 16)
                {
                    uint16_t* ptr = reinterpret_cast<uint16_t*>(address);
                    *ptr = x & 1 ? fill_color_16_0_ : fill_color_16_1_;
                }
                else
                {
                    uint32_t* ptr = reinterpret_cast<uint32_t*>(address);
                    *ptr = fill_color_32_;
                }
                break;
            }
        }
    }

    // TODO: using uint8s is technically not correct
    uint8_t combine(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
    {
        return (a - b) * c / 0xFF + d;
    }

    void RDP::color_combiner(int cycle)
    {
        uint8_t r = combine(*color_sub_a_[cycle], *color_sub_b_[cycle], *color_multiplier_[cycle],
                            *color_adder_[cycle]);
        uint8_t g = combine(*color_sub_a_[cycle] >> 8, *color_sub_b_[cycle] >> 8,
                            *color_multiplier_[cycle] >> 8, *color_adder_[cycle] >> 8);
        uint8_t b = combine(*color_sub_a_[cycle] >> 16, *color_sub_b_[cycle] >> 16,
                            *color_multiplier_[cycle] >> 16, *color_adder_[cycle] >> 16);
        uint8_t a = combine(*alpha_sub_a_[cycle], *alpha_sub_b_[cycle], *alpha_multiplier_[cycle],
                            *alpha_adder_[cycle]);
        combined_color_ = (a << 24) | (b << 16) | (g << 8) | r;
        combined_alpha_ = a << 24 | a << 16 | a << 8 | a;
    }

    uint32_t RDP::blender(int cycle)
    {
        uint32_t color1, color2;
        uint8_t multiplier1, multiplier2;

        switch (blender_1a_[cycle] & 0b11)
        {
            case 0:
                color1 = combined_color_;
                break;
            case 1:
                color1 = framebuffer_color_;
                break;
            case 2:
                color1 = blend_color_;
                break;
            case 3:
                color1 = fog_color_;
                break;
        }

        switch (blender_2a_[cycle] & 0b11)
        {
            case 0:
                color2 = combined_color_;
                break;
            case 1:
                color2 = framebuffer_color_;
                break;
            case 2:
                color2 = blend_color_;
                break;
            case 3:
                color2 = fog_color_;
                break;
        }

        switch (blender_1b_[cycle] & 0b11)
        {
            case 0:
                multiplier1 = combined_alpha_;
                break;
            case 1:
                multiplier1 = fog_alpha_;
                break;
            case 2:
                multiplier1 = shade_alpha_;
                break;
            case 3:
                multiplier1 = 0x00;
                break;
        }

        switch (blender_2b_[cycle] & 0b11)
        {
            case 0:
                multiplier2 = ~multiplier1;
                break;
            case 1:
                // Apparently this is coverage
                // The bits in the framebuffer that are normally used for alpha are actually
                // coverage use 0 for now
                multiplier2 = 0x00;
                break;
            case 2:
                multiplier2 = 0xFF;
                break;
            case 3:
                multiplier2 = 0x00;
                break;
        }

        if (multiplier1 + multiplier2 == 0)
        {
            Logger::WarnOnce("Blender division by zero - blender settings: {} {} {} {}",
                             blender_1a_[cycle], blender_2a_[cycle], blender_1b_[cycle],
                             blender_2b_[cycle]);
            multiplier1 = 0xFF;
        }

        uint8_t r = (((color1 >> 0) & 0xFF) * multiplier1 + ((color2 >> 0) & 0xFF) * multiplier2) /
                    (multiplier1 + multiplier2);
        uint8_t g = (((color1 >> 8) & 0xFF) * multiplier1 + ((color2 >> 8) & 0xFF) * multiplier2) /
                    (multiplier1 + multiplier2);
        uint8_t b =
            (((color1 >> 16) & 0xFF) * multiplier1 + ((color2 >> 16) & 0xFF) * multiplier2) /
            (multiplier1 + multiplier2);

        return (0xFF << 24) | (b << 16) | (g << 8) | r;
    }

    bool RDP::depth_test(int x, int y, uint32_t z, uint16_t dz)
    {
        enum DepthMode { Opaque, Interpenetrating, Transparent, Decal };

        if (z_compare_en_)
        {
            uint32_t old_depth = z_get(x, y);
            uint16_t old_dz = dz_get(x, y);
            bool pass = false;

            switch (z_mode_ & 0b11)
            {
                // TODO: other depth modes
                case Opaque:
                {
                    // TODO: there's also some coverage stuff going on here normally
                    pass = (old_depth == 0x3ffff) || (z < old_depth);
                    break;
                }
                case Interpenetrating:
                {
                    Logger::WarnOnce("Interpenetrating depth mode not implemented");
                    pass = (old_depth == 0x3ffff) || (z < old_depth);
                    break;
                }
                case Transparent:
                {
                    pass = (old_depth == 0x3ffff) || (z < old_depth);
                    break;
                }
                case Decal:
                {
                    pass = hydra::abs(z - old_depth) <= hydra::max(dz, old_dz);
                    break;
                }
            }
            // TODO: std::unreachable
            return pass;
        }
        else
        {
            return true;
        }
    }

    uint32_t RDP::z_get(int x, int y)
    {
        uintptr_t address = reinterpret_cast<uintptr_t>(rdram_ptr_) + zbuffer_dram_address_ +
                            (y * framebuffer_width_ + x) * 2;
        uint16_t* ptr = reinterpret_cast<uint16_t*>(address);
        uint32_t decompressed = z_decompress_lut_[(*ptr >> 2) & 0x3FFF];
        return decompressed;
    }

    uint8_t RDP::dz_get(int x, int y)
    {
        uintptr_t address = reinterpret_cast<uintptr_t>(rdram_ptr_) + zbuffer_dram_address_ +
                            (y * framebuffer_width_ + x) * 2;
        uint16_t* ptr = reinterpret_cast<uint16_t*>(address);
        return *ptr & 0b11;
    }

    void RDP::z_set(int x, int y, uint32_t z)
    {
        z &= 0x3FFFF;
        uintptr_t address = reinterpret_cast<uintptr_t>(rdram_ptr_) + zbuffer_dram_address_ +
                            (y * framebuffer_width_ + x) * 2;
        uint16_t* ptr = reinterpret_cast<uint16_t*>(address);
        uint16_t compressed = z_compress_lut_[z & 0x3FFFF];
        *ptr = compressed;
    }

    constexpr std::array<uint8_t, 8> z_shifts = {6, 5, 4, 3, 2, 1, 0, 0};

    uint32_t RDP::z_compress(uint32_t z)
    {
        // count the most significant set bits and that is the exponent
        uint32_t exponent = std::countl_one(
            // mask bits so that we only count up to 7 bits
            (z & 0b111111100000000000)
            // shift them to the start for countl_one
            << 14);
        uint32_t mantissa = (z >> z_shifts[exponent]) & 0b111'1111'1111;
        return (exponent << 11) | mantissa;
    }

    uint32_t RDP::z_decompress(uint32_t z)
    {
        uint32_t exponent = (z >> 11) & 0x7;
        uint32_t mantissa = z & 0x7FF;
        // shift mantissa to the msb, shift back by the exponent which
        // will create n bits where n = exponent, then move those bits
        // to the correct position
        uint32_t bits = (!!exponent << 31) >> exponent;
        bits >>= 13;
        bits |= mantissa << z_shifts[exponent];
        return bits & 0x3FFFF;
    }

    void RDP::fetch_texels(int texel, int tile, int32_t s, int32_t t)
    {
        TileDescriptor& td = tiles_[tile];
        if (td.clamp_s)
        {
            auto max_s = ((td.sh >> 2) - (td.sl >> 2)) & 0x3ff;
            s = std::clamp(s, 0, max_s);
        }
        else if (!td.mirror_s)
            s &= td.mask_s;
        if (td.clamp_t)
        {
            auto max_t = ((td.th >> 2) - (td.tl >> 2)) & 0x3ff;
            t = std::clamp(t, 0, max_t);
        }
        else if (!td.mirror_t)
            t &= td.mask_t;
        switch (td.format)
        {
            case Format::RGBA:
            {
                switch (td.size)
                {
                    case 16:
                    {
                        uint16_t address = (td.tmem_address + (t * td.line_width) + s * 2) & 0xFFF;
                        if (t & 1)
                        {
                            address ^= 0b10;
                        }
                        uint8_t byte1 = tmem_[address & 0xFFF];
                        uint8_t byte2 = tmem_[(address + 1) & 0xFFF];
                        texel_color_[texel] = rgba16_to_rgba32((byte1 << 8) | byte2);
                        texel_alpha_[texel] = (texel_color_[texel] >> 24) |
                                              (texel_color_[texel] >> 16) |
                                              (texel_color_[texel] >> 8) | texel_color_[texel];
                        break;
                    }
                    case 32:
                    {
                        uint16_t address = (td.tmem_address + (t * td.line_width) + s * 2) & 0xFFF;
                        uint8_t byte1 = tmem_[address & 0xFFF];
                        uint8_t byte2 = tmem_[(address + 1) & 0xFFF];
                        uint8_t byte3 = tmem_[(address + 2) & 0xFFF];
                        uint8_t byte4 = tmem_[(address + 3) & 0xFFF];
                        texel_color_[texel] = (byte1 << 24) | (byte2 << 16) | (byte3 << 8) | byte4;
                        texel_alpha_[texel] = (texel_color_[texel] >> 24) |
                                              (texel_color_[texel] >> 16) |
                                              (texel_color_[texel] >> 8) | texel_color_[texel];
                        break;
                    }
                    default:
                    {
                        Logger::WarnOnce("Unimplemented texture size for RGBA: {}",
                                         static_cast<int>(td.size));
                        break;
                    }
                }
                break;
            }
            case Format::IA:
            {
                switch (td.size)
                {
                    case 4:
                    {
                        uint16_t address = (td.tmem_address + (t * td.line_width) + s / 2) & 0xFFF;
                        uint8_t ia = tmem_[address & 0xFFF];
                        ia = (s & 1) ? (ia & 0xF) : (ia >> 4);
                        uint8_t i = ia & 0xE;
                        i = (i << 4) | (i << 1) | (i >> 2);
                        uint8_t a = (ia & 0x1) ? 0xFF : 0;
                        texel_color_[texel] = (a << 24) | (i << 16) | (i << 8) | i;
                        texel_alpha_[texel] = (a << 24) | (a << 16) | (a << 8) | a;
                        break;
                    }
                    case 8:
                    {
                        uint16_t address = (td.tmem_address + (t * td.line_width) + s) & 0xFFF;
                        uint8_t ia = tmem_[address & 0xFFF];
                        uint8_t i = (ia >> 4) | (ia & 0xF0);
                        uint8_t a = (ia & 0xF) | (ia << 4);
                        texel_color_[texel] = (a << 24) | (i << 16) | (i << 8) | i;
                        texel_alpha_[texel] = (a << 24) | (a << 16) | (a << 8) | a;
                        break;
                    }
                    case 16:
                    {
                        uint16_t address = (td.tmem_address + (t * td.line_width) + s * 2) & 0xFFF;
                        if (t & 1)
                        {
                            address ^= 0b10;
                        }
                        uint8_t i = tmem_[address & 0xFFF];
                        uint8_t a = tmem_[(address + 1) & 0xFFF];
                        texel_color_[texel] = (a << 24) | (i << 16) | (i << 8) | i;
                        texel_alpha_[texel] = (a << 24) | (a << 16) | (a << 8) | a;
                        break;
                    }
                    default:
                    {
                        Logger::WarnOnce("Unimplemented texture size for IA: {}",
                                         static_cast<int>(td.size));
                        break;
                    }
                }
                break;
            }
            case Format::I:
            {
                switch (td.size)
                {
                    case 4:
                    {
                        uint16_t address = (td.tmem_address + (t * td.line_width) + s / 2) & 0xFFF;
                        uint8_t i = tmem_[address & 0xFFF];
                        if (s & 1)
                        {
                            i &= 0xF;
                        }
                        else
                        {
                            i >>= 4;
                        }
                        texel_color_[texel] = (i << 24) | (i << 16) | (i << 8) | i;
                        texel_alpha_[texel] = texel_color_[texel];
                        break;
                    }
                    case 8:
                    {
                        uint16_t address = (td.tmem_address + (t * td.line_width) + s) & 0xFFF;
                        uint8_t i = tmem_[address & 0xFFF];
                        texel_color_[texel] = (i << 24) | (i << 16) | (i << 8) | i;
                        texel_alpha_[texel] = texel_color_[texel];
                        break;
                    }
                    default:
                    {
                        Logger::WarnOnce("Unimplemented texture size for I: {}",
                                         static_cast<int>(td.size));
                        break;
                    }
                }
                break;
            }
            default:
            {
                Logger::WarnOnce("Unimplemented texture format: {}", static_cast<int>(td.format));
            }
        }
    }

    void RDP::get_noise()
    {
        auto r = irand(&seed_);
        noise_color_ = (r << 24) | (r << 16) | (r << 8) | r;
    }

    void RDP::load_tile(const LoadTileCommand& command)
    {
        TileDescriptor& td = tiles_[command.tile];
        uint32_t x_start = command.SL >> 2;
        uint32_t x_end = command.SH >> 2;
        uint32_t y_start = command.TL >> 2;
        uint32_t y_end = command.TH >> 2;
        uint32_t dram_offset, tmem_offset;

        td.sl = command.SL;
        td.sh = command.SH;
        td.tl = command.TL;
        td.th = command.TH;

        switch (td.size)
        {
            case 8:
            {
                for (uint32_t y = y_start; y <= y_end; ++y)
                {
                    for (uint32_t x = x_start; x <= x_end; ++x)
                    {
                        dram_offset = texture_dram_address_latch_ + (y * texture_width_latch_ + x);
                        tmem_offset =
                            (td.tmem_address + ((y - y_start) * td.line_width) + (x - x_start));
                        tmem_.at(tmem_offset) = rdram_ptr_[dram_offset];
                    }
                }
                break;
            }
            case 16:
            {
                for (uint32_t y = y_start; y <= y_end; ++y)
                {
                    for (uint32_t x = x_start; x <= x_end; ++x)
                    {
                        dram_offset =
                            texture_dram_address_latch_ + (y * texture_width_latch_ + x) * 2;
                        tmem_offset = (td.tmem_address + ((y - y_start) * td.line_width) +
                                       ((x - x_start) * 2));
                        if (y & 1)
                        {
                            tmem_offset ^= 0b10;
                        }
                        tmem_.at(tmem_offset) = rdram_ptr_[dram_offset];
                        tmem_.at(tmem_offset + 1) = rdram_ptr_[dram_offset + 1];
                    }
                }
                break;
            }
            case 32:
            {
                for (uint32_t y = y_start; y <= y_end; ++y)
                {
                    for (uint32_t x = x_start; x <= x_end; ++x)
                    {
                        dram_offset =
                            texture_dram_address_latch_ + (y * texture_width_latch_ + x) * 4;
                        tmem_offset = (td.tmem_address + ((y - y_start) * td.line_width) +
                                       ((x - x_start) * 2));
                        tmem_.at(tmem_offset) = rdram_ptr_[dram_offset];
                        tmem_.at(tmem_offset + 1) = rdram_ptr_[dram_offset + 1];
                        tmem_.at(tmem_offset + 2) = rdram_ptr_[dram_offset + 2];
                        tmem_.at(tmem_offset + 3) = rdram_ptr_[dram_offset + 3];
                    }
                }
                break;
            }
            default:
            {
                Logger::WarnOnce("Unimplemented LoadTile command with size: {}", td.size);
                break;
            }
        }
    }

    void RDP::init_depth_luts()
    {
        for (int i = 0; i < 0x4000; i++)
        {
            z_decompress_lut_[i] = z_decompress(i);
        }

        for (int i = 0; i < 0x40000; i++)
        {
            // the 2 lower bits along with 2 more from the rdrams 9th bit
            // are used to store the depth delta
            z_compress_lut_[i] = z_compress(i) << 2;
        }
    }

    // Taken from angrylion's RDP, modified to use a vector of uint64_t
    // and to return a Primitive, used to compare spans in real time
    // for debugging purposes
    // Code: https://github.com/ata4/angrylion-rdp-plus
    Primitive RDP::get_angrylion_primitive(const EdgewalkerInput& input)
    {
        Primitive ret;

        int j = 0;
        int xleft = 0, xright = 0, xleft_inc = 0, xright_inc = 0;
        int r = 0, g = 0, b = 0, a = 0, z = 0, s = 0, t = 0, w = 0;
        int drdx = 0, dgdx = 0, dbdx = 0, dadx = 0, dzdx = 0, dsdx = 0, dtdx = 0, dwdx = 0;
        int drdy = 0, dgdy = 0, dbdy = 0, dady = 0, dzdy = 0, dsdy = 0, dtdy = 0, dwdy = 0;
        int drde = 0, dgde = 0, dbde = 0, dade = 0, dzde = 0, dsde = 0, dtde = 0, dwde = 0;
        int flip = 0;
        int32_t yl = 0, ym = 0, yh = 0;
        int32_t xl = 0, xm = 0, xh = 0;
        int32_t dxldy = 0, dxhdy = 0, dxmdy = 0;

        flip = input.right_major;

#define SIGN(x, numb) (((x) & ((1 << (numb)) - 1)) | -((x) & (1 << ((numb)-1))))
        yl = input.yl;
        ym = input.ym;
        yh = input.yh;

        xl = input.xl;
        xh = input.xh;
        xm = input.xm;

        dxldy = input.slopel;
        dxhdy = input.slopeh;
        dxmdy = input.slopem;

        r = input.r;
        g = input.g;
        b = input.b;
        a = input.a;
        drdx = input.DrDx;
        dgdx = input.DgDx;
        dbdx = input.DbDx;
        dadx = input.DaDx;
        drde = input.DrDe;
        dgde = input.DgDe;
        dbde = input.DbDe;
        dade = input.DaDe;
        drdy = input.DrDy;
        dgdy = input.DgDy;
        dbdy = input.DbDy;
        dady = input.DaDy;

        s = input.s;
        t = input.t;
        w = input.w;
        dsdx = input.DsDx;
        dtdx = input.DtDx;
        dwdx = input.DwDx;
        dsde = input.DsDe;
        dtde = input.DtDe;
        dwde = input.DwDe;
        dsdy = input.DsDy;
        dtdy = input.DtDy;
        dwdy = input.DwDy;

        z = input.z;
        dzdx = input.DzDx;
        dzde = input.DzDe;
        dzdy = input.DzDy;

        ret.DrDx = drdx & ~0x1F;
        ret.DgDx = dgdx & ~0x1F;
        ret.DbDx = dbdx & ~0x1F;
        ret.DaDx = dadx & ~0x1F;
        ret.DsDx = dsdx & ~0x1F;
        ret.DtDx = dtdx & ~0x1F;
        ret.DwDx = dwdx & ~0x1F;
        ret.DzDx = dzdx;

        xleft_inc = (dxmdy >> 2) & ~0x1;
        xright_inc = (dxhdy >> 2) & ~0x1;

        xright = xh & ~0x1;
        xleft = xm & ~0x1;

        int k = 0;

        int dsdiff, dtdiff, dwdiff, drdiff, dgdiff, dbdiff, dadiff, dzdiff;
        int sign_dxhdy = (input.slopeh & 0x80000000) != 0;

        int dsdeh, dtdeh, dwdeh, drdeh, dgdeh, dbdeh, dadeh, dzdeh, dsdyh, dtdyh, dwdyh, drdyh,
            dgdyh, dbdyh, dadyh, dzdyh;
        int do_offset = !(sign_dxhdy ^ flip);

        if (do_offset)
        {
            dsdeh = dsde & ~0x1ff;
            dtdeh = dtde & ~0x1ff;
            dwdeh = dwde & ~0x1ff;
            drdeh = drde & ~0x1ff;
            dgdeh = dgde & ~0x1ff;
            dbdeh = dbde & ~0x1ff;
            dadeh = dade & ~0x1ff;
            dzdeh = dzde & ~0x1ff;

            dsdyh = dsdy & ~0x1ff;
            dtdyh = dtdy & ~0x1ff;
            dwdyh = dwdy & ~0x1ff;
            drdyh = drdy & ~0x1ff;
            dgdyh = dgdy & ~0x1ff;
            dbdyh = dbdy & ~0x1ff;
            dadyh = dady & ~0x1ff;
            dzdyh = dzdy & ~0x1ff;

            dsdiff = dsdeh - (dsdeh >> 2) - dsdyh + (dsdyh >> 2);
            dtdiff = dtdeh - (dtdeh >> 2) - dtdyh + (dtdyh >> 2);
            dwdiff = dwdeh - (dwdeh >> 2) - dwdyh + (dwdyh >> 2);
            drdiff = drdeh - (drdeh >> 2) - drdyh + (drdyh >> 2);
            dgdiff = dgdeh - (dgdeh >> 2) - dgdyh + (dgdyh >> 2);
            dbdiff = dbdeh - (dbdeh >> 2) - dbdyh + (dbdyh >> 2);
            dadiff = dadeh - (dadeh >> 2) - dadyh + (dadyh >> 2);
            dzdiff = dzdeh - (dzdeh >> 2) - dzdyh + (dzdyh >> 2);
        }
        else
            dsdiff = dtdiff = dwdiff = drdiff = dgdiff = dbdiff = dadiff = dzdiff = 0;

        int xfrac = 0;

        int dsdxh, dtdxh, dwdxh, drdxh, dgdxh, dbdxh, dadxh, dzdxh;
        if (cycle_type_ != CycleType::Copy)
        {
            dsdxh = (dsdx >> 8) & ~1;
            dtdxh = (dtdx >> 8) & ~1;
            dwdxh = (dwdx >> 8) & ~1;
            drdxh = (drdx >> 8) & ~1;
            dgdxh = (dgdx >> 8) & ~1;
            dbdxh = (dbdx >> 8) & ~1;
            dadxh = (dadx >> 8) & ~1;
            dzdxh = (dzdx >> 8) & ~1;
        }
        else
            dsdxh = dtdxh = dwdxh = drdxh = dgdxh = dbdxh = dadxh = dzdxh = 0;

#define ADJUST_ATTR_PRIM()                                                   \
    {                                                                        \
        ret.spans[j].s = ((s & ~0x1ff) + dsdiff - (xfrac * dsdxh)) & ~0x3ff; \
        ret.spans[j].t = ((t & ~0x1ff) + dtdiff - (xfrac * dtdxh)) & ~0x3ff; \
        ret.spans[j].w = ((w & ~0x1ff) + dwdiff - (xfrac * dwdxh)) & ~0x3ff; \
        ret.spans[j].r = ((r & ~0x1ff) + drdiff - (xfrac * drdxh)) & ~0x3ff; \
        ret.spans[j].g = ((g & ~0x1ff) + dgdiff - (xfrac * dgdxh)) & ~0x3ff; \
        ret.spans[j].b = ((b & ~0x1ff) + dbdiff - (xfrac * dbdxh)) & ~0x3ff; \
        ret.spans[j].a = ((a & ~0x1ff) + dadiff - (xfrac * dadxh)) & ~0x3ff; \
        ret.spans[j].z = ((z & ~0x1ff) + dzdiff - (xfrac * dzdxh)) & ~0x3ff; \
    }

#define ADDVALUES_PRIM() \
    {                    \
        s += dsde;       \
        t += dtde;       \
        w += dwde;       \
        r += drde;       \
        g += dgde;       \
        b += dbde;       \
        a += dade;       \
        z += dzde;       \
    }

        int32_t maxxmx = 0, minxmx = 0, maxxhx = 0, minxhx = 0;

        int spix = 0;
        int ycur = yh & ~3;
        int ldflag = (sign_dxhdy ^ flip) ? 0 : 3;
        int invaly = 1;
        // int length = 0;
        int32_t xrsc = 0, xlsc = 0, stickybit = 0;
        int32_t yllimit = 0, yhlimit = 0;
        if (yl & 0x2000)
            yllimit = 1;
        else if (yl & 0x1000)
            yllimit = 0;
        else
            yllimit = (yl & 0xfff) < scissor_yl_;
        yllimit = yllimit ? yl : scissor_yl_;

        int ylfar = yllimit | 3;
        if ((yl >> 2) > (ylfar >> 2))
            ylfar += 4;
        else if ((yllimit >> 2) >= 0 && (yllimit >> 2) < 1023)
            ret.spans[(yllimit >> 2) + 1].valid = 0;

        if (yh & 0x2000)
            yhlimit = 0;
        else if (yh & 0x1000)
            yhlimit = 1;
        else
            yhlimit = (yh >= scissor_yh_);
        yhlimit = yhlimit ? yh : scissor_yh_;

        int yhclose = yhlimit & ~3;

        int32_t clipxlshift = scissor_xl_ << 1;
        int32_t clipxhshift = scissor_xh_ << 1;
        int allover = 1, allunder = 1, cur_over = 0, cur_under = 0;
        int allinval = 1;
        int32_t curcross = 0;

        xfrac = ((xright >> 8) & 0xff);

        if (flip)
        {
            for (k = ycur; k <= ylfar; k++)
            {
                if (k == ym)
                {
                    xleft = xl & ~1;
                    xleft_inc = (dxldy >> 2) & ~1;
                }

                spix = k & 3;

                if (k >= yhclose)
                {
                    invaly = k < yhlimit || k >= yllimit;

                    j = k >> 2;

                    if (spix == 0)
                    {
                        maxxmx = 0;
                        minxhx = 0xfff;
                        allover = allunder = 1;
                        allinval = 1;
                    }

                    stickybit = ((xright >> 1) & 0x1fff) > 0;
                    xrsc = ((xright >> 13) & 0x1ffe) | stickybit;

                    cur_under =
                        ((xright & 0x8000000) || (xrsc < clipxhshift && !(xright & 0x4000000)));

                    xrsc = cur_under ? clipxhshift : (((xright >> 13) & 0x3ffe) | stickybit);
                    cur_over = ((xrsc & 0x2000) || (xrsc & 0x1fff) >= clipxlshift);
                    xrsc = cur_over ? clipxlshift : xrsc;
                    allover &= cur_over;
                    allunder &= cur_under;

                    stickybit = ((xleft >> 1) & 0x1fff) > 0;
                    xlsc = ((xleft >> 13) & 0x1ffe) | stickybit;
                    cur_under =
                        ((xleft & 0x8000000) || (xlsc < clipxhshift && !(xleft & 0x4000000)));
                    xlsc = cur_under ? clipxhshift : (((xleft >> 13) & 0x3ffe) | stickybit);
                    cur_over = ((xlsc & 0x2000) || (xlsc & 0x1fff) >= clipxlshift);
                    xlsc = cur_over ? clipxlshift : xlsc;
                    allover &= cur_over;
                    allunder &= cur_under;

                    curcross = ((xleft ^ (1 << 27)) & (0x3fff << 14)) <
                               ((xright ^ (1 << 27)) & (0x3fff << 14));

                    invaly |= curcross;
                    allinval &= invaly;

                    if (!invaly)
                    {
                        maxxmx = (((xlsc >> 3) & 0xfff) > maxxmx) ? (xlsc >> 3) & 0xfff : maxxmx;
                        minxhx = (((xrsc >> 3) & 0xfff) < minxhx) ? (xrsc >> 3) & 0xfff : minxhx;
                    }

                    if (spix == ldflag)
                    {
                        xfrac = (xright >> 8) & 0xff;
                        ADJUST_ATTR_PRIM();
                    }

                    if (spix == 3)
                    {
                        ret.spans[j].max_x = maxxmx;
                        ret.spans[j].min_x = minxhx;
                        ret.spans[j].valid = !allinval && !allover && !allunder;
                    }
                }

                if (spix == 3)
                {
                    ADDVALUES_PRIM();
                }

                xleft += xleft_inc;
                xright += xright_inc;
            }
        }
        else
        {
            for (k = ycur; k <= ylfar; k++)
            {
                if (k == ym)
                {
                    xleft = xl & ~1;
                    xleft_inc = (dxldy >> 2) & ~1;
                }

                spix = k & 3;

                if (k >= yhclose)
                {
                    invaly = k < yhlimit || k >= yllimit;
                    j = k >> 2;

                    if (spix == 0)
                    {
                        maxxhx = 0;
                        minxmx = 0xfff;
                        allover = allunder = 1;
                        allinval = 1;
                    }

                    stickybit = ((xright >> 1) & 0x1fff) > 0;
                    xrsc = ((xright >> 13) & 0x1ffe) | stickybit;
                    cur_under =
                        ((xright & 0x8000000) || (xrsc < clipxhshift && !(xright & 0x4000000)));
                    xrsc = cur_under ? clipxhshift : (((xright >> 13) & 0x3ffe) | stickybit);
                    cur_over = ((xrsc & 0x2000) || (xrsc & 0x1fff) >= clipxlshift);
                    xrsc = cur_over ? clipxlshift : xrsc;
                    allover &= cur_over;
                    allunder &= cur_under;

                    stickybit = ((xleft >> 1) & 0x1fff) > 0;
                    xlsc = ((xleft >> 13) & 0x1ffe) | stickybit;
                    cur_under =
                        ((xleft & 0x8000000) || (xlsc < clipxhshift && !(xleft & 0x4000000)));
                    xlsc = cur_under ? clipxhshift : (((xleft >> 13) & 0x3ffe) | stickybit);
                    cur_over = ((xlsc & 0x2000) || (xlsc & 0x1fff) >= clipxlshift);
                    xlsc = cur_over ? clipxlshift : xlsc;
                    allover &= cur_over;
                    allunder &= cur_under;

                    curcross = ((xright ^ (1 << 27)) & (0x3fff << 14)) <
                               ((xleft ^ (1 << 27)) & (0x3fff << 14));

                    invaly |= curcross;
                    allinval &= invaly;

                    if (!invaly)
                    {
                        minxmx = (((xlsc >> 3) & 0xfff) < minxmx) ? (xlsc >> 3) & 0xfff : minxmx;
                        maxxhx = (((xrsc >> 3) & 0xfff) > maxxhx) ? (xrsc >> 3) & 0xfff : maxxhx;
                    }

                    if (spix == ldflag)
                    {
                        xfrac = (xright >> 8) & 0xff;
                        ADJUST_ATTR_PRIM();
                    }

                    if (spix == 3)
                    {
                        ret.spans[j].min_x = minxmx;
                        ret.spans[j].max_x = maxxhx;
                        ret.spans[j].valid = !allinval && !allover && !allunder;
                    }
                }

                if (spix == 3)
                {
                    ADDVALUES_PRIM();
                }

                xleft += xleft_inc;
                xright += xright_inc;
            }
        }
#undef ADDVALUES_PRIM
#undef ADJUST_ATTR_PRIM
#undef SIGN
        ret.y_start = yhlimit >> 2;
        ret.y_end = yllimit >> 2;
        return ret;
    }

    void RDP::check_primitive(const Primitive& my_primitive, const EdgewalkerInput& input,
                              const std::vector<uint64_t>& data)
    {
        Primitive angrylion_primitive = get_angrylion_primitive(input);
        if (my_primitive.y_start != angrylion_primitive.y_start)
        {
            Logger::Fatal("Primitive mismatch: y_start - expected: {}, actual: {}",
                          angrylion_primitive.y_start, my_primitive.y_start);
        }
        if (my_primitive.y_end != angrylion_primitive.y_end)
        {
            Logger::Fatal("Primitive mismatch: y_end - expected: {}, actual: {}",
                          angrylion_primitive.y_end, my_primitive.y_end);
        }
        for (int32_t i = angrylion_primitive.y_start; i < angrylion_primitive.y_end; i++)
        {
            std::string problem = "";
            if (my_primitive.spans[i].min_x != angrylion_primitive.spans[i].min_x)
            {
                problem = "min_x";
            }

            if (my_primitive.spans[i].max_x != angrylion_primitive.spans[i].max_x)
            {
                problem = "max_x";
            }

            if (my_primitive.spans[i].valid != angrylion_primitive.spans[i].valid)
            {
                problem = "valid";
            }

            if (my_primitive.spans[i].r != angrylion_primitive.spans[i].r ||
                my_primitive.spans[i].g != angrylion_primitive.spans[i].g ||
                my_primitive.spans[i].b != angrylion_primitive.spans[i].b ||
                my_primitive.spans[i].a != angrylion_primitive.spans[i].a ||
                my_primitive.DrDx != angrylion_primitive.DrDx ||
                my_primitive.DgDx != angrylion_primitive.DgDx ||
                my_primitive.DbDx != angrylion_primitive.DbDx ||
                my_primitive.DaDx != angrylion_primitive.DaDx)
            {
                Logger::Warn("Primitive mismatch: span {} color - expected: {:08x}, {:08x}, "
                             "{:08x}, {:08x} actual: {:08x}, {:08x}, {:08x}, {:08x}",
                             i, angrylion_primitive.spans[i].r, angrylion_primitive.spans[i].g,
                             angrylion_primitive.spans[i].b, angrylion_primitive.spans[i].a,
                             my_primitive.spans[i].r, my_primitive.spans[i].g,
                             my_primitive.spans[i].b, my_primitive.spans[i].a);
                printf("DrDx: %08x, DgDx: %08x, DbDx: %08x, DaDx: %08x\n", angrylion_primitive.DrDx,
                       angrylion_primitive.DgDx, angrylion_primitive.DbDx,
                       angrylion_primitive.DaDx);
                printf("My DrDx: %08x, DgDx: %08x, DbDx: %08x, DaDx: %08x\n", my_primitive.DrDx,
                       my_primitive.DgDx, my_primitive.DbDx, my_primitive.DaDx);
            }

            if (my_primitive.spans[i].s != angrylion_primitive.spans[i].s ||
                my_primitive.spans[i].t != angrylion_primitive.spans[i].t ||
                my_primitive.spans[i].w != angrylion_primitive.spans[i].w)
            {
                Logger::Fatal("Primitive mismatch: span {} texture - expected: {:08x}, {:08x}, "
                              "{:08x}, actual: {:08x}, {:08x}, {:08x}",
                              i, angrylion_primitive.spans[i].s, angrylion_primitive.spans[i].t,
                              angrylion_primitive.spans[i].w, my_primitive.spans[i].s,
                              my_primitive.spans[i].t, my_primitive.spans[i].w);
            }

            if (my_primitive.spans[i].z != angrylion_primitive.spans[i].z ||
                my_primitive.DzDx != angrylion_primitive.DzDx)
            {
                problem = fmt::format("z expected: {:08x}, actual: {:08x}",
                                      angrylion_primitive.spans[i].z, my_primitive.spans[i].z);
            }

            if (problem != "")
            {
                printf("Dumping problematic primitive:\n");
                for (uint64_t block : data)
                {
                    printf("0x%016lx, ", block);
                }
                printf("\n");
                printf("Scissor YH: %08x, YL: %08x, XH: %08x, XL: %08x\n", scissor_yh_, scissor_yl_,
                       scissor_xh_, scissor_xl_);
                printf("Cycle type: %d\n", cycle_type_);
                printf("Y start: %d, Y end: %d\n", my_primitive.y_start, my_primitive.y_end);
                Logger::Fatal("Primitive mismatch: span {} - {}", i, problem);
            }
        }
    }

    EdgewalkerInput RDP::triangle_get_edgewalker_input(const std::vector<uint64_t>& data,
                                                       bool shade, bool texture, bool depth)
    {
        EdgewalkerInput ret;

        EdgeCoefficientsCommand command;
        EdgeCoefficients edgel, edgem, edgeh;
        command.full = data[0];
        edgel.full = data[1];
        edgeh.full = data[2];
        edgem.full = data[3];

        ret.tile_index = command.tile;

        // Sign extend the 14 bit values
        ret.yh = static_cast<int16_t>(command.YH << 2) >> 2;
        ret.ym = static_cast<int16_t>(command.YM << 2) >> 2;
        ret.yl = static_cast<int16_t>(command.YL << 2) >> 2;

        // Sign extend the 28 bit values
        ret.xl = (edgel.X << 4) >> 4;
        ret.xm = (edgem.X << 4) >> 4;
        ret.xh = (edgeh.X << 4) >> 4;

        // Sign extend the 30 bit values
        ret.slopel = (edgel.slope << 2) >> 2;
        ret.slopem = (edgem.slope << 2) >> 2;
        ret.slopeh = (edgeh.slope << 2) >> 2;

        ret.right_major = command.lft;

        int next_block = 4;

        if (shade)
        {
            ret.r = (((data[next_block] >> 48) & 0xFFFF) << 16) |
                    ((data[next_block + 2] >> 48) & 0xFFFF);
            ret.g = (((data[next_block] >> 32) & 0xFFFF) << 16) |
                    ((data[next_block + 2] >> 32) & 0xFFFF);
            ret.b = (((data[next_block] >> 16) & 0xFFFF) << 16) |
                    ((data[next_block + 2] >> 16) & 0xFFFF);
            ret.a =
                (((data[next_block] >> 0) & 0xFFFF) << 16) | ((data[next_block + 2] >> 0) & 0xFFFF);

            ret.DrDx = (((data[next_block + 1] >> 48) & 0xFFFF) << 16) |
                       ((data[next_block + 3] >> 48) & 0xFFFF);
            ret.DgDx = (((data[next_block + 1] >> 32) & 0xFFFF) << 16) |
                       ((data[next_block + 3] >> 32) & 0xFFFF);
            ret.DbDx = (((data[next_block + 1] >> 16) & 0xFFFF) << 16) |
                       ((data[next_block + 3] >> 16) & 0xFFFF);
            ret.DaDx = (((data[next_block + 1] >> 0) & 0xFFFF) << 16) |
                       ((data[next_block + 3] >> 0) & 0xFFFF);

            ret.DrDe = (((data[next_block + 4] >> 48) & 0xFFFF) << 16) |
                       ((data[next_block + 6] >> 48) & 0xFFFF);
            ret.DgDe = (((data[next_block + 4] >> 32) & 0xFFFF) << 16) |
                       ((data[next_block + 6] >> 32) & 0xFFFF);
            ret.DbDe = (((data[next_block + 4] >> 16) & 0xFFFF) << 16) |
                       ((data[next_block + 6] >> 16) & 0xFFFF);
            ret.DaDe = (((data[next_block + 4] >> 0) & 0xFFFF) << 16) |
                       ((data[next_block + 6] >> 0) & 0xFFFF);

            ret.DrDy = (((data[next_block + 5] >> 48) & 0xFFFF) << 16) |
                       ((data[next_block + 7] >> 48) & 0xFFFF);
            ret.DgDy = (((data[next_block + 5] >> 32) & 0xFFFF) << 16) |
                       ((data[next_block + 7] >> 32) & 0xFFFF);
            ret.DbDy = (((data[next_block + 5] >> 16) & 0xFFFF) << 16) |
                       ((data[next_block + 7] >> 16) & 0xFFFF);
            ret.DaDy = (((data[next_block + 5] >> 0) & 0xFFFF) << 16) |
                       ((data[next_block + 7] >> 0) & 0xFFFF);

            ret.DrDx &= ~0x1f;
            ret.DgDx &= ~0x1f;
            ret.DbDx &= ~0x1f;
            ret.DaDx &= ~0x1f;

            next_block += 8;
        }

        if (texture)
        {
            ret.s = (((data[next_block] >> 48) & 0xFFFF) << 16) |
                    ((data[next_block + 2] >> 48) & 0xFFFF);
            ret.t = (((data[next_block] >> 32) & 0xFFFF) << 16) |
                    ((data[next_block + 2] >> 32) & 0xFFFF);
            ret.w = (((data[next_block] >> 16) & 0xFFFF) << 16) |
                    ((data[next_block + 2] >> 16) & 0xFFFF);

            ret.DsDx = (((data[next_block + 1] >> 48) & 0xFFFF) << 16) |
                       ((data[next_block + 3] >> 48) & 0xFFFF);
            ret.DtDx = (((data[next_block + 1] >> 32) & 0xFFFF) << 16) |
                       ((data[next_block + 3] >> 32) & 0xFFFF);
            ret.DwDx = (((data[next_block + 1] >> 16) & 0xFFFF) << 16) |
                       ((data[next_block + 3] >> 16) & 0xFFFF);

            ret.DsDe = (((data[next_block + 4] >> 48) & 0xFFFF) << 16) |
                       ((data[next_block + 6] >> 48) & 0xFFFF);
            ret.DtDe = (((data[next_block + 4] >> 32) & 0xFFFF) << 16) |
                       ((data[next_block + 6] >> 32) & 0xFFFF);
            ret.DwDe = (((data[next_block + 4] >> 16) & 0xFFFF) << 16) |
                       ((data[next_block + 6] >> 16) & 0xFFFF);

            ret.DsDy = (((data[next_block + 5] >> 48) & 0xFFFF) << 16) |
                       ((data[next_block + 7] >> 48) & 0xFFFF);
            ret.DtDy = (((data[next_block + 5] >> 32) & 0xFFFF) << 16) |
                       ((data[next_block + 7] >> 32) & 0xFFFF);
            ret.DwDy = (((data[next_block + 5] >> 16) & 0xFFFF) << 16) |
                       ((data[next_block + 7] >> 16) & 0xFFFF);

            ret.DsDx &= ~0x1f;
            ret.DtDx &= ~0x1f;
            ret.DwDx &= ~0x1f;

            next_block += 8;
        }

        if (depth)
        {
            ret.z = data[next_block] >> 32;
            ret.DzDx = data[next_block] & 0xFFFF'FFFF;
            ret.DzDe = data[next_block + 1] >> 32;
            ret.DzDy = data[next_block + 1] & 0xFFFF'FFFF;
        }

        return ret;
    }

    template <bool Texture, bool Flip>
    EdgewalkerInput RDP::rectangle_get_edgewalker_input(const std::vector<uint64_t>& data)
    {
        // Rectangles are simply triangles with slopes = 0 in the RDP
        EdgewalkerInput ret;

        RectangleCommand command;
        command.full = data[0];

        if (cycle_type_ == CycleType::Copy || cycle_type_ == CycleType::Fill)
        {
            command.yl |= 3;
        }

        int32_t xl_integer = command.xl >> 2;
        int32_t xh_integer = command.xh >> 2;

        ret.xl = (xl_integer << 16) | ((command.xl & 0b11) << 14);
        ret.xm = (xl_integer << 16) | ((command.xl & 0b11) << 14);
        ret.xh = (xh_integer << 16) | ((command.xh & 0b11) << 14);

        ret.yl = command.yl;
        ret.ym = command.yl;
        ret.yh = command.yh;

        ret.right_major = true;
        ret.tile_index = command.tile;

        if constexpr (Texture)
        {
            ret.s = data[1] >> 48;
            ret.t = (data[1] >> 32) & 0xFFFF;
            int32_t DsDx = (int16_t)((data[1] >> 16) & 0xFFFF);
            int32_t DtDy = (int16_t)(data[1] & 0xFFFF);

            // s.5.10, we need to shift left by 6 to move the integer part to the top 16 bits
            DsDx <<= 6;
            DtDy <<= 6;

            if (cycle_type_ == CycleType::Copy)
            {
                DsDx >>= 2;
            }

            ret.DsDx = DsDx;
            ret.DtDe = DtDy;
            ret.DtDy = DtDy;
        }

        return ret;
    }

    template <class T>
    T clear_subpixels(T value)
    {
        return value & ~3;
    }

    template <class T>
    T set_subpixels(T value)
    {
        return value | 3;
    }

    Primitive RDP::edgewalker(const EdgewalkerInput& input)
    {
        Primitive primitive;

        primitive.tile_index = input.tile_index;

        int32_t xh = input.xh, xm = input.xm, xl = input.xl;
        int32_t yh = input.yh, ym = input.ym, yl = input.yl;

        int32_t r = input.r, g = input.g, b = input.b, a = input.a;
        int32_t s = input.s, t = input.t, w = input.w;
        int32_t z = input.z;

        int32_t DrDe = input.DrDe, DgDe = input.DgDe, DbDe = input.DbDe, DaDe = input.DaDe;
        int32_t DsDe = input.DsDe, DtDe = input.DtDe, DwDe = input.DwDe;
        int32_t DzDe = input.DzDe;

        int32_t DrDx = 0, DgDx = 0, DbDx = 0, DaDx = 0;
        int32_t DsDx = 0, DtDx = 0, DwDx = 0;
        int32_t DzDx = 0;

        int32_t DrDy = input.DrDy, DgDy = input.DgDy, DbDy = input.DbDy, DaDy = input.DaDy;
        int32_t DsDy = input.DsDy, DtDy = input.DtDy, DwDy = input.DwDy;
        int32_t DzDy = input.DzDy;

        int32_t DrDiff = 0, DgDiff = 0, DbDiff = 0, DaDiff = 0;
        int32_t DsDiff = 0, DtDiff = 0, DwDiff = 0;
        int32_t DzDiff = 0;

        primitive.DrDx = input.DrDx & ~0x1f;
        primitive.DgDx = input.DgDx & ~0x1f;
        primitive.DbDx = input.DbDx & ~0x1f;
        primitive.DaDx = input.DaDx & ~0x1f;
        primitive.DsDx = input.DsDx & ~0x1f;
        primitive.DtDx = input.DtDx & ~0x1f;
        primitive.DwDx = input.DwDx & ~0x1f;
        primitive.DzDx = input.DzDx;

        primitive.right_major = input.right_major;

        bool sign_slopeh = input.slopeh & 0x80000000;
        if (cycle_type_ != CycleType::Copy)
        {
            DrDx = (input.DrDx >> 8) & ~1;
            DgDx = (input.DgDx >> 8) & ~1;
            DbDx = (input.DbDx >> 8) & ~1;
            DaDx = (input.DaDx >> 8) & ~1;
            DsDx = (input.DsDx >> 8) & ~1;
            DtDx = (input.DtDx >> 8) & ~1;
            DwDx = (input.DwDx >> 8) & ~1;
            DzDx = (input.DzDx >> 8) & ~1;
        }

        bool do_offset = !(sign_slopeh ^ input.right_major);

        if (do_offset)
        {
            int32_t dsdeh = DsDe & ~0x1ff;
            int32_t dtdeh = DtDe & ~0x1ff;
            int32_t dwdeh = DwDe & ~0x1ff;
            int32_t drdeh = DrDe & ~0x1ff;
            int32_t dgdeh = DgDe & ~0x1ff;
            int32_t dbdeh = DbDe & ~0x1ff;
            int32_t dadeh = DaDe & ~0x1ff;
            int32_t dzdeh = DzDe & ~0x1ff;

            int32_t dsdyh = DsDy & ~0x1ff;
            int32_t dtdyh = DtDy & ~0x1ff;
            int32_t dwdyh = DwDy & ~0x1ff;
            int32_t drdyh = DrDy & ~0x1ff;
            int32_t dgdyh = DgDy & ~0x1ff;
            int32_t dbdyh = DbDy & ~0x1ff;
            int32_t dadyh = DaDy & ~0x1ff;
            int32_t dzdyh = DzDy & ~0x1ff;

            DrDiff = drdeh - (drdeh >> 2) - drdyh + (drdyh >> 2);
            DgDiff = dgdeh - (dgdeh >> 2) - dgdyh + (dgdyh >> 2);
            DbDiff = dbdeh - (dbdeh >> 2) - dbdyh + (dbdyh >> 2);
            DaDiff = dadeh - (dadeh >> 2) - dadyh + (dadyh >> 2);
            DsDiff = dsdeh - (dsdeh >> 2) - dsdyh + (dsdyh >> 2);
            DtDiff = dtdeh - (dtdeh >> 2) - dtdyh + (dtdyh >> 2);
            DwDiff = dwdeh - (dwdeh >> 2) - dwdyh + (dwdyh >> 2);
            DzDiff = dzdeh - (dzdeh >> 2) - dzdyh + (dzdyh >> 2);
        }

        bool use_scissor_low = false, use_scissor_high = false;

        int32_t y_start = clear_subpixels(yh);

        if (yh & 0x2000)
        {
            use_scissor_high = true;
        }
        else if (yh & 0x1000)
        {
            use_scissor_high = false;
        }
        else
        {
            use_scissor_high = yh < scissor_yh_;
        }

        int32_t yh_limit = use_scissor_high ? scissor_yh_ : yh;

        if (yl & 0x2000)
        {
            use_scissor_low = false;
        }
        else if (yl & 0x1000)
        {
            use_scissor_low = true;
        }
        else
        {
            use_scissor_low = yl >= scissor_yl_;
        }

        int32_t yl_limit = use_scissor_low ? scissor_yl_ : yl;

        // Top and bottom of the primitive
        int32_t y_top = clear_subpixels(yh_limit);
        int32_t y_bottom = set_subpixels(yl_limit);

        int32_t x_left_inc = (input.slopem >> 2) & ~1;
        int32_t x_right_inc = (input.slopeh >> 2) & ~1;

        int32_t x_left = xm & ~1;
        int32_t x_right = xh & ~1;

        int32_t scissor_xl_shift = scissor_xl_ << 1;
        int32_t scissor_xh_shift = scissor_xh_ << 1;

        int32_t span_leftmost = 0, span_rightmost = 0;

        primitive.y_start = y_top >> 2;
        primitive.y_end = y_bottom >> 2;

        Span current_span;

        // To check whether every subpixel is inside the scissor
        bool all_invalid = true, all_over = true, all_under = true;

        // We start from y_start instead of y_top because we need to
        // edgewalk the shade/texture/depth values regardless of whether
        // we're drawing the current span
        for (int32_t y = y_start; y <= y_bottom; y++)
        {
            if (y == ym)
            {
                // At the middle point x_left becomes xl and uses slope L
                x_left = xl;
                x_left_inc = (input.slopel >> 2) & ~1;
            }

            uint8_t subpixel = y & 3;
            if (y >= y_top)
            {
                bool invalid_y = y < yh_limit || y >= yl_limit;
                int32_t integer_y = y >> 2;

                if (subpixel == 0)
                {
                    current_span = Span();

                    if (input.right_major)
                    {
                        span_leftmost = 0;
                        span_rightmost = 0xFFF;
                    }
                    else
                    {
                        span_leftmost = 0xFFF;
                        span_rightmost = 0;
                    }
                    all_invalid = true;
                    all_over = true;
                    all_under = true;
                }

                // Check if the current subpixel is inside the scissor
                // XH/XL are 16.16 fixed point, so shifting by 14 would convert the lower 12
                // bits to 10.2 fixed point Since scissor_**_shift is shifted to the left by 1,
                // we need to shift the XH/XL values by 13 instead of 14, which would convert
                // them to 10.2 fixed point
                bool round_bit = ((x_right >> 1) & 0x1fff) > 0;
                int32_t x_right_clipped = ((x_right >> 13) & 0x1FFE) | round_bit;
                bool cur_under = ((x_right & 0x8000000) ||
                                  (x_right_clipped < scissor_xh_shift && !(x_right & 0x4000000)));
                x_right_clipped =
                    cur_under ? scissor_xh_shift : (((x_right >> 13) & 0x3FFE) | round_bit);
                bool cur_over =
                    ((x_right_clipped & 0x2000) || (x_right_clipped & 0x1FFF) >= scissor_xl_shift);
                x_right_clipped = cur_over ? scissor_xl_shift : x_right_clipped;

                all_under &= cur_under;
                all_over &= cur_over;

                round_bit = ((x_left >> 1) & 0x1fff) > 0;
                int32_t x_left_clipped = ((x_left >> 13) & 0x1FFE) | round_bit;
                cur_under = ((x_left & 0x8000000) ||
                             (x_left_clipped < scissor_xh_shift && !(x_left & 0x4000000)));
                x_left_clipped =
                    cur_under ? scissor_xh_shift : (((x_left >> 13) & 0x3FFE) | round_bit);
                cur_over =
                    ((x_left_clipped & 0x2000) || (x_left_clipped & 0x1FFF) >= scissor_xl_shift);
                x_left_clipped = cur_over ? scissor_xl_shift : x_left_clipped;

                all_under &= cur_under;
                all_over &= cur_over;

                x_right_clipped >>= 3;
                x_right_clipped &= 0xFFF;
                x_left_clipped >>= 3;
                x_left_clipped &= 0xFFF;

                bool curcross;

                // We don't want to continue drawing the primitive if
                // it intersects
                if (!input.right_major)
                {
                    curcross = ((x_left ^ (1 << 27)) & (0x3fff << 14)) >
                               ((x_right ^ (1 << 27)) & (0x3fff << 14));
                }
                else
                {
                    curcross = ((x_left ^ (1 << 27)) & (0x3fff << 14)) <
                               ((x_right ^ (1 << 27)) & (0x3fff << 14));
                }
                all_invalid &= invalid_y | curcross;

                if (!invalid_y && !curcross)
                {
                    if (input.right_major)
                    {
                        if (x_left_clipped > span_leftmost)
                        {
                            span_leftmost = x_left_clipped;
                        }

                        if (x_right_clipped < span_rightmost)
                        {
                            span_rightmost = x_right_clipped;
                        }
                    }
                    else
                    {
                        if (x_left_clipped < span_leftmost)
                        {
                            span_leftmost = x_left_clipped;
                        }

                        if (x_right_clipped > span_rightmost)
                        {
                            span_rightmost = x_right_clipped;
                        }
                    }
                }

                int primitive_load_subpixel = (sign_slopeh ^ input.right_major) ? 0 : 3;

                if (subpixel == primitive_load_subpixel)
                {
                    int32_t x_frac = (x_right >> 8) & 0xFF;
                    current_span.r = ((r & ~0x1FF) + DrDiff - (x_frac * DrDx)) & ~0x3FF;
                    current_span.g = ((g & ~0x1FF) + DgDiff - (x_frac * DgDx)) & ~0x3FF;
                    current_span.b = ((b & ~0x1FF) + DbDiff - (x_frac * DbDx)) & ~0x3FF;
                    current_span.a = ((a & ~0x1FF) + DaDiff - (x_frac * DaDx)) & ~0x3FF;
                    current_span.s = ((s & ~0x1FF) + DsDiff - (x_frac * DsDx)) & ~0x3FF;
                    current_span.t = ((t & ~0x1FF) + DtDiff - (x_frac * DtDx)) & ~0x3FF;
                    current_span.w = ((w & ~0x1FF) + DwDiff - (x_frac * DwDx)) & ~0x3FF;
                    current_span.z = ((z & ~0x1FF) + DzDiff - (x_frac * DzDx)) & ~0x3FF;
                }

                if (subpixel == 3)
                {
                    current_span.min_x = span_leftmost;
                    current_span.max_x = span_rightmost;
                    if (input.right_major)
                    {
                        std::swap(current_span.min_x, current_span.max_x);
                    }
                    current_span.valid = !all_invalid && !all_over && !all_under;
                    primitive.spans[integer_y] = current_span;
                }
            }

            if (subpixel == 3)
            {
                r += DrDe;
                g += DgDe;
                b += DbDe;
                a += DaDe;
                s += DsDe;
                t += DtDe;
                w += DwDe;
                z += DzDe;
            }

            x_left += x_left_inc;
            x_right += x_right_inc;
        }

        return primitive;
    }

    hydra_inline uint8_t color_clamp(uint16_t color)
    {
        switch ((color >> 7) & 3)
        {
            case 0:
            case 1:
                return color & 0xff;
            case 2:
                return 0xff;
            default:
                return 0;
        }
    }

    hydra_inline int32_t z_correct(int32_t z)
    {
        z >>= 3;

        switch ((z >> 17) & 3)
        {
            case 0:
            case 1:
                return z & 0x3ffff;
            case 2:
                return 0x3ffff;
            default:
                return 0;
        }
    }

    void RDP::render_primitive(const Primitive& primitive)
    {
        int32_t x_start = 0, x_inc = 0;
        int32_t DzDx = primitive.DzDx;
        int32_t DrDx = primitive.DrDx;
        int32_t DgDx = primitive.DgDx;
        int32_t DbDx = primitive.DbDx;
        int32_t DaDx = primitive.DaDx;

        if (z_source_sel_)
        {
            DzDx = 0;
        }

        for (int y = primitive.y_start; y <= primitive.y_end; y++)
        {
            const Span& span = primitive.spans[y];
            if (!span.valid)
                continue;

            int32_t r = span.r;
            int32_t g = span.g;
            int32_t b = span.b;
            int32_t a = span.a;
            int32_t s = span.s;
            int32_t t = span.t;
            int32_t w = span.w;
            int32_t z = z_source_sel_ ? primitive_depth_ : span.z;

            if (primitive.right_major)
            {
                x_start = span.min_x;
                x_inc = 1;
            }
            else
            {
                x_start = span.max_x;
                x_inc = -1;
            }

            int32_t x = x_start;
            int length = span.max_x - span.min_x;

            for (int i = 0; i <= length; i++)
            {
                uint8_t r8 = color_clamp(r >> 16);
                uint8_t g8 = color_clamp(g >> 16);
                uint8_t b8 = color_clamp(b >> 16);
                uint8_t a8 = color_clamp(a >> 16);

                shade_color_ = (a8 << 24) | (b8 << 16) | (g8 << 8) | r8;
                shade_alpha_ = (a8 << 24) | (a8 << 16) | (a8 << 8) | a8;

                get_noise();

                int32_t z_cur = z_correct((z >> 10) & 0x3f'ffff);
                if (depth_test(x, y, z_cur, 0))
                {
                    auto [s_cur, t_cur] = perspective_correction_func_(s, t, w);
                    fetch_texels(0, primitive.tile_index, s_cur, t_cur);
                    fetch_texels(1, primitive.tile_index, s_cur, t_cur);
                    draw_pixel(x, y);

                    if (z_update_en_)
                    {
                        z_set(x, y, z_cur);
                    }
                }

                z += DzDx * x_inc;
                r += DrDx * x_inc;
                g += DgDx * x_inc;
                b += DbDx * x_inc;
                a += DaDx * x_inc;
                s += primitive.DsDx * x_inc;
                t += primitive.DtDx * x_inc;
                w += primitive.DwDx * x_inc;
                x += x_inc;
            }
        }
    }
} // namespace hydra::N64