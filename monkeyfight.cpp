#include <SPI.h>
#include <GD.h>
#include "Game.h"

TGame	Game;

void setup()
{
	//	init gameduino
	GD.begin();

	//	init systems


	//	init game
	Game.Init();
}

void loop()
{
	//	update game
	Game.Update();

	//	system (GD push)

	//	render
    GD.waitvblank();

	//	system update (GD pull)
}
