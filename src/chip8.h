#pragma once

#include <iostream>
#include <cstdint>
#include <fstream>
#include <string>
#include <random>
#include <cmath>
#include <raylib.h>

#define ERROR(message) \
{ \
	std::cerr << "\nERROR: " << message << "\n" << __FILE__ << ":" << __LINE__ << "\n\n"; \
}

#define ERROR_OPCODE(opcode) \
{ \
	std::cerr << "\nERROR: Unknown opcode `0x" << std::hex << opcode << std::dec << "`\n"; \
}

#define DISPLAY_MEMORY_WIDTH  64
#define DISPLAY_MEMORY_HEIGHT 32
#define START_ADDRESS 0x200
#define FONT_SET_SIZE 80
#define FONT_SET_START_ADDRESS 0x50
#define CPU_FREQ   500
#define TIMER_FREQ 60

#define WINDOW_TITLE "CHIP-8 Ahoy!"
#define TARGET_FPS 60

const uint8_t font_set[FONT_SET_SIZE] = {
	0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	0x20, 0x60, 0x20, 0x20, 0x70, // 1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

const uint8_t keymap[16] = {
    KEY_X, KEY_ONE, KEY_TWO, KEY_THREE,
    KEY_Q, KEY_W, KEY_E, KEY_A,
    KEY_S, KEY_D, KEY_Z, KEY_C,
    KEY_FOUR, KEY_R, KEY_F, KEY_V
};

enum class Watchpoint_Type
{
	MEMORY, REGISTER
};

struct Watchpoint
{
	Watchpoint_Type type;
	uint16_t address;

	//uint8_t  reg;	// Register variable abbreviated to "reg" as if:
					//                  		    	             A) ...it's 1872 and we may run low on video memory--80 columns by 24 lines--if we don't name variables like dolts who can't tell the difference between a remote control and a keyboard.
					//                                               B) ...the programmer typing the code is so busy developing actual good software that the remaining "-ister" would waste their precious time. The world would stop being a better place because they're not writing good software, which for sure is not some framework code cobbled together. (I'm not a good programmer)
					// 							          	         C) ...well, not really an "as if", because "register" is indeed a reserved keyword in C/C++.

	//uint8_t reginald;

	uint8_t reggie; // Even better
	uint8_t value;
};

struct Debugger
{
	bool is_paused   = false;
	bool is_stepping = false;
	Watchpoint watchpoints[16] = {};
	size_t watchpoint_count = 0;
};

class Chip8
{
public:
	uint8_t  registers[16] = {};
	uint8_t  memory[4096]  = {};
	uint8_t  stack_pointer = 0;
	uint8_t  delay_timer = 0;
	uint8_t  sound_timer = 0;
	uint8_t  keypad[16]  = {};
	uint8_t  display_scale = 1;
	uint16_t index = 0;
	uint16_t program_counter = 0;
	uint16_t stack[16] = {};
	uint16_t opcode = 0;
	uint32_t display_memory[DISPLAY_MEMORY_WIDTH * DISPLAY_MEMORY_HEIGHT] = {};
	Sound	 sound;

	Debugger debugger = {};


	Chip8(uint8_t scale);
	~Chip8();

	bool load_rom(const char* filepath);
	uint8_t random_byte();
	void op_00e0(); // Clear screen
	void op_00ee(); // Return
	void op_1nnn(); // Jump to address NNN (goto)
	void op_2nnn(); // Call subroutine at NNN
	void op_3xnn(); // if (VX == NN)
	void op_4xnn(); // if (VX != NN)
	void op_5xy0(); // if (VX == VY)
	void op_6xnn(); // Set VX = NN
	void op_7xnn(); // Set VX += NN
	void op_8xy0(); // Set VX = VY
	void op_8xy1(); // Set VX |= VY
	void op_8xy2(); // Set VX &= VY
	void op_8xy3(); // Set VX ^= VY
	void op_8xy4(); // Set VX += VY, VF = carry
	void op_8xy5(); // Set VX -= VY, VF = 0 for underflow, 1 if not
	void op_8xy6(); // VX >>= 1, stores the LSB of VX into VF before the shift
	void op_8xy7(); // VX = VY - VX, VF = 0 for underflow, 1 if not
	void op_8xye(); // VX <<= 1, stores the MSB of VX into VF before the shift, or 0 if unset
	void op_9xy0(); // if (VX != VY)
	void op_annn(); // Set index to NNN
	void op_bnnn(); // PC = V0 + NNN
	void op_cxnn(); // VX = rand() % NN
	void op_dxyn(); // draw(VX, VY, N: bytes)
	void op_ex9e(); // if (pressed_key == VX)
	void op_exa1(); // if (pressed_key != VX)
	void op_fx07(); // Set VX = delay_timer
	void op_fx0a(); // Wait for a key press and then VX = pressed_key
	void op_fx15(); // delay_timer = VX
	void op_fx18(); // sound_timer = VX
	void op_fx1e(); // index += VX
	void op_fx29(); // index = location of sprite for digit VX
	void op_fx33(); // Store BCD representation of VX in index, index + 1 and index + 2
	void op_fx55(); // Store V0 to VX in memory, starting at index
	void op_fx65(); // Load V0 to VX with values from memory, starting at index
	void cycle();

	Sound create_sound(unsigned int sample_rate = 44100, unsigned int length_ms = 100, float frequency = 440.0f);
	void process_audio();
	void process_input();
	void render() const;
	void process_debug_input(uint16_t address = 0, uint8_t reggie = 0, uint8_t value = 0);
};