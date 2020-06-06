cmake_minimum_required(VERSION 3.8)
project (blitrog)
include (../../ooblit.cmake)
blit_executable (blitrog game.cpp StartScreen.cpp GameScene.cpp PauseScene.cpp)
blit_add_ooblit (blitrog)
target_include_directories(blitrog
	PUBLIC ../../32blit
		../../ooblit
		)

