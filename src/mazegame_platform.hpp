#ifndef MAZEGAME_HPP

// #include "mazegame_essentials.hpp"

struct GameInput
{
	float horizontal;
	float vertical;
};

void GameUpdateAndRender(GameInput * input);

#define MAZEGAME_HPP
#endif