cmake_minimum_required(VERSION 3.8)
project (jumpingjack)
include (../../ooblit.cmake)
blit_executable (jumpingjack game.cpp StartScreen.cpp GameScene.cpp PauseScene.cpp)
blit_add_ooblit (jumpingjack)
target_include_directories(jumpingjack
	PUBLIC ../../32blit
		../../ooblit
		)

