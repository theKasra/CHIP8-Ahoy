#include "chip8.h"

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		std::cout << "\nCHIP8-Ahoy!\n";
		std::cerr << "Usage:   " << argv[0] << " <display_scale> <ROM>\n";
		std::cerr << "Example: " << argv[0] << " 20 ROMs/Pong.ch8\n\n";
		return EXIT_FAILURE;
	}

	Chip8 chip(std::stoi(argv[1]));
	if (!chip.load_rom(argv[2]))
	{
		return EXIT_FAILURE;
	}

	double last_time	= GetTime();
	double current_time = 0;
	double delta_time	= 0;
	double cycle_accum	= 0;
	double cycle_time	= 0;
	double timer_accum	= 0;
	double timer_time	= 0;

	while (!WindowShouldClose())
	{
		current_time = GetTime();
		delta_time = current_time - last_time;
		last_time = current_time;

		cycle_accum += delta_time;
		timer_accum += delta_time;

		cycle_time = 1.0f / CPU_FREQ;
		while (cycle_accum >= cycle_time)
		{
			chip.cycle();
			cycle_accum -= cycle_time;
		}

		timer_time = 1.0f / TIMER_FREQ;
		while (timer_accum >= timer_time)
		{
			if (chip.delay_timer > 0) chip.delay_timer--;
			if (chip.sound_timer > 0) chip.sound_timer--;
			timer_accum -= timer_time;
		}

		chip.process_input();
		// chip.process_debug_input();
		chip.process_audio();

		chip.render();
	}

	return 0;
}