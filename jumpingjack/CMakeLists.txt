cmake_minimum_required(VERSION 3.1)
project (jumpingjack)
include (../../ooblit.cmake)
blit_executable (jumpingjack game.cpp GameScene.cpp PauseScene.cpp)
target_include_directories(BlitHalSTM32
	PUBLIC ../../32blit
		../../ooblit
)
