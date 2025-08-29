#include "chip8.h"

Chip8::Chip8(uint8_t scale)
	:display_scale(scale)
{
	program_counter = START_ADDRESS;
	for (size_t i = 0; i < FONT_SET_START_ADDRESS; i++) memory[FONT_SET_START_ADDRESS + i] = font_set[i];

	InitAudioDevice();
	sound = create_sound();

	InitWindow(DISPLAY_MEMORY_WIDTH * display_scale, DISPLAY_MEMORY_HEIGHT * display_scale, WINDOW_TITLE);
	SetTargetFPS(TARGET_FPS);
}

Chip8::~Chip8()
{
	UnloadSound(sound);
	CloseAudioDevice();
	CloseWindow();
}

bool Chip8::load_rom(const char* filepath)
{
	std::ifstream file(filepath, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		ERROR("Failed to load ROM.");
		return false;
	}

	std::streampos size = file.tellg();
	char* buffer = new char[size];
	file.seekg(0, std::ios::beg);
	file.read(buffer, size);

	file.close();

	for (size_t i = 0; i < size; i++) memory[START_ADDRESS + i] = buffer[i];

	delete[] buffer;
	return true;
}

uint8_t Chip8::random_byte()
{
	static std::mt19937 range(std::random_device{}());
	static std::uniform_int_distribution<int> dist(0, 255);
	return static_cast<uint8_t>(dist(range));
}

void Chip8::op_00e0() { memset(display_memory, 0, sizeof(display_memory)); }

void Chip8::op_00ee() { program_counter = stack[--stack_pointer]; }

void Chip8::op_1nnn() { program_counter = opcode & 0x0fffu; }

void Chip8::op_2nnn()
{
	stack[stack_pointer++] = program_counter;
	program_counter = opcode & 0x0fffu;
}

void Chip8::op_3xnn()
{
	uint8_t vx   = (opcode & 0x0f00u) >> 8u;
	uint8_t byte =  opcode & 0x00ffu;
	if (registers[vx] == byte) program_counter += 2;
}

void Chip8::op_4xnn()
{
	uint8_t vx   = (opcode & 0x0f00u) >> 8u;
	uint8_t byte =  opcode & 0x00ffu;
	if (registers[vx] != byte) program_counter += 2;
}

void Chip8::op_5xy0()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	if (registers[vx] == registers[vy]) program_counter += 2;
}

void Chip8::op_6xnn()
{
	uint8_t vx    = (opcode & 0x0f00u) >> 8u;
	uint8_t byte  =  opcode & 0x00ffu;
	registers[vx] =  byte;
}

void Chip8::op_7xnn()
{
	uint8_t vx     = (opcode & 0x0f00u) >> 8u;
	uint8_t byte   =  opcode & 0x00ffu;
	registers[vx] += byte;
}

void Chip8::op_8xy0()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	registers[vx] = registers[vy];
}

void Chip8::op_8xy1()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	registers[vx] |= registers[vy];
}

void Chip8::op_8xy2()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	registers[vx] &= registers[vy];
}

void Chip8::op_8xy3()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	registers[vx] ^= registers[vy];
}

void Chip8::op_8xy4()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	uint16_t sum = registers[vx] + registers[vy];

	if (sum > 255u) registers[0xf] = 1;
	else registers[0xf] = 0;

	registers[vx] = sum & 0xffu;
}

void Chip8::op_8xy5()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;

	if (registers[vx] < registers[vy]) registers[0xf] = 0;
	else registers[0xf] = 1;

	registers[vx] -= registers[vy];
}

void Chip8::op_8xy6()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	registers[0xf] = registers[vx] & 0x1u;
	registers[vx] >>= 1;
}

void Chip8::op_8xy7()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;

	if (registers[vx] < registers[vy]) registers[0xf] = 1;
	else registers[0xf] = 0;

	registers[vx] = registers[vy] - registers[vx];
}

void Chip8::op_8xye()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	registers[0xf] = (registers[vx] & 0x80u) >> 7u;
	registers[vx] <<= 1;
}

void Chip8::op_9xy0()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	if (registers[vx] != registers[vy]) program_counter += 2;
}

void Chip8::op_annn() { index = opcode & 0x0fffu; }

void Chip8::op_bnnn() { program_counter = registers[0] + (opcode & 0x0fffu); }

void Chip8::op_cxnn()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t byte = opcode & 0x00ffu;
	registers[vx] = random_byte() & byte;
}

void Chip8::op_dxyn()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t vy = (opcode & 0x00f0u) >> 4u;
	uint8_t height = opcode & 0x000fu;

	uint8_t x_pos = registers[vx] % DISPLAY_MEMORY_WIDTH;
	uint8_t y_pos = registers[vy] % DISPLAY_MEMORY_HEIGHT;

	registers[0xf] = 0;

	for (size_t row = 0; row < height; row++)
	{
		uint8_t sprite_byte = memory[index + row];
		for (size_t col = 0; col < 8; col++)
		{
			uint8_t sprite_pixel = sprite_byte & (0x80u >> col);
			uint32_t* screen_pixel = &display_memory[(y_pos + row) * DISPLAY_MEMORY_WIDTH + (x_pos + col)];

			if (sprite_pixel)
			{
				if (*screen_pixel == 0xffffffff) registers[0xf] = 1;
				*screen_pixel ^= 0xffffffff;
			}
		}
	}
}

void Chip8::op_ex9e()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	if (keypad[registers[vx]]) program_counter += 2;
}

void Chip8::op_exa1()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	if (!keypad[registers[vx]]) program_counter += 2;
}

void Chip8::op_fx07()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	registers[vx] = delay_timer;
}

void Chip8::op_fx0a()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	bool is_key_pressed = false;

	for (size_t i = 0; i < 16; i++)
	{
		if (keypad[i])
		{
			registers[vx] = i;
			is_key_pressed = true;
			break;
		}
	}

	if (!is_key_pressed) program_counter -= 2;
}

void Chip8::op_fx15()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	delay_timer = registers[vx];
}

void Chip8::op_fx18()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	sound_timer = registers[vx];
}

void Chip8::op_fx1e()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	index += registers[vx];
}

void Chip8::op_fx29()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	index = FONT_SET_START_ADDRESS + (5 * registers[vx]);
}

void Chip8::op_fx33()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	uint8_t value = registers[vx];
	memory[index + 2] = value % 10;		value /= 10;
	memory[index + 1] = value % 10;		value /= 10;
	memory[index]     = value % 10;
}

void Chip8::op_fx55()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	for (size_t i = 0; i <= vx; i++) memory[index + i] = registers[i];
}

void Chip8::op_fx65()
{
	uint8_t vx = (opcode & 0x0f00u) >> 8u;
	for (size_t i = 0; i <= vx; i++) registers[i] = memory[index + i];
}

void Chip8::cycle()
{
	if (debugger.is_paused && !debugger.is_stepping) return;

	opcode = (memory[program_counter] << 8u) | memory[program_counter + 1];
	program_counter += 2;

	// An unsavoury switch case
	switch (opcode & 0xf000u)
	{
		case 0x0000:
		{
			switch (opcode & 0x00ffu)
			{
				case 0x00e0: op_00e0(); break;
				case 0x00ee: op_00ee(); break;
				default: ERROR_OPCODE(opcode);
			} break;
		}
		case 0x1000: op_1nnn(); break;
		case 0x2000: op_2nnn(); break;
		case 0x3000: op_3xnn(); break;
		case 0x4000: op_4xnn(); break;
		case 0x5000: op_5xy0(); break;
		case 0x6000: op_6xnn(); break;
		case 0x7000: op_7xnn(); break;
		case 0x8000:
		{
			switch (opcode & 0x000fu) //ck this
			{
				case 0x0000: op_8xy0(); break;
				case 0x0001: op_8xy1(); break;
				case 0x0002: op_8xy2(); break;
				case 0x0003: op_8xy3(); break;
				case 0x0004: op_8xy4(); break;
				case 0x0005: op_8xy5(); break;
				case 0x0006: op_8xy6(); break;
				case 0x0007: op_8xy7(); break;
				case 0x000e: op_8xye(); break;
				default: ERROR_OPCODE(opcode);
			} break;
		}
		case 0x9000: op_9xy0(); break;
		case 0xa000: op_annn(); break;
		case 0xb000: op_bnnn(); break;
		case 0xc000: op_cxnn(); break;
		case 0xd000: op_dxyn(); break;
		case 0xe000:
		{
			switch (opcode & 0x00ffu)
			{
				case 0x009e: op_ex9e(); break;
				case 0x00a1: op_exa1(); break;
				default: ERROR_OPCODE(opcode);
			} break;
		}
		case 0xf000:
		{
			switch (opcode & 0x00ffu)
			{
				case 0x0007: op_fx07(); break;
				case 0x000a: op_fx0a(); break;
				case 0x0015: op_fx15(); break;
				case 0x0018: op_fx18(); break;
				case 0x001e: op_fx1e(); break;
				case 0x0029: op_fx29(); break;
				case 0x0033: op_fx33(); break;
				case 0x0055: op_fx55(); break;
				case 0x0065: op_fx65(); break;
				default: ERROR_OPCODE(opcode);
			} break;
		}
		default: ERROR_OPCODE(opcode);
	}

	// if (delay_timer > 0) delay_timer--;
	// if (sound_timer > 0) sound_timer--;

	for (size_t i = 0; i < debugger.watchpoint_count; i++)
	{
		Watchpoint& watchpoint = debugger.watchpoints[i];
		if (watchpoint.type == Watchpoint_Type::MEMORY && memory[watchpoint.address] == watchpoint.value)
		{
			debugger.is_paused = true;
			break;
		}
		else if (watchpoint.type == Watchpoint_Type::REGISTER && registers[watchpoint.reggie] == watchpoint.value)
		{
			debugger.is_paused = true;
			break;
		}
	}

	if (debugger.is_stepping)
	{
		debugger.is_paused = true;
		debugger.is_stepping = false;
	}
}

// Raylib's playing/stopping sound crashes in games like Pong
void Chip8::process_audio()
{
	if (sound_timer > 0)
	{
		if (!IsSoundPlaying(sound)) PlaySound(sound);
	}
	else
	{
		if (IsSoundPlaying(sound))  StopSound(sound);
	}
}

void Chip8::process_input()
{
	for (size_t i = 0; i < 16; i++) keypad[i] = IsKeyDown(keymap[i]);
}

void Chip8::render() const
{
	BeginDrawing();

	ClearBackground(BLACK);
	for (size_t y = 0; y < DISPLAY_MEMORY_HEIGHT; y++)
	{
		for (size_t x = 0; x < DISPLAY_MEMORY_WIDTH; x++)
		{
			uint32_t pixel = display_memory[y * DISPLAY_MEMORY_WIDTH + x];
			Color color = (pixel == 0xffffffff) ? WHITE : BLACK;
			DrawRectangle(x * display_scale, y * display_scale, display_scale, display_scale, color);
		}
	}

	EndDrawing();
}

Sound Chip8::create_sound(unsigned int sample_rate, unsigned int length_ms, float frequency)
{
	unsigned int frame_count = sample_rate * length_ms / 1000;
	short* samples = new short[frame_count];

	for (size_t i = 0; i < frame_count; i++)
	{
		float t = (float)i / sample_rate;
		samples[i] = (short)(32767 * (std::sin(2 * PI * frequency * t) > 0 ? 1.0f : -1.0f));
	}

	Wave wave = {};
	wave.frameCount = frame_count;
	wave.sampleRate = sample_rate;
	wave.sampleSize = 16;
	wave.channels = 1;
	wave.data = samples;

	Sound sound = LoadSoundFromWave(wave);

	UnloadWave(wave);
	return sound;
}

// Ehhh... Maybe later
void Chip8:: process_debug_input(uint16_t address, uint8_t reggie, uint8_t value)
{
	if (IsKeyDown(KEY_H))
	{
		// Pause
		debugger.is_paused = !debugger.is_paused;
	}
	else if (IsKeyDown(KEY_G))
	{
		// Step
		debugger.is_paused = false;
		debugger.is_stepping = true;
	}
	else if (IsKeyDown(KEY_T))
	{
		// delete watchpoint(s)
	}
	else if (IsKeyDown(KEY_B))
	{
		// set memory watchpoint
		if (debugger.watchpoint_count < 16) debugger.watchpoints[debugger.watchpoint_count++] = {Watchpoint_Type::MEMORY, address, 0, value};
	}
	else if (IsKeyDown(KEY_N))
	{
		// set register watchpoint
		if (debugger.watchpoint_count < 16) debugger.watchpoints[debugger.watchpoint_count++] = {Watchpoint_Type::REGISTER, 0, reggie, value};
	}
	else if (IsKeyDown(KEY_Y))
	{
		// change the value of a memory address
		memory[address] = value;
	}
	else if (IsKeyDown(KEY_U))
	{
		// change the value of a register
		registers[reggie] = value;
	}
}
