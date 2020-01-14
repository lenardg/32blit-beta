file(GLOB SOURCES *.cpp)
add_library(OOBlitEngine STATIC ${SOURCES})

target_include_directories(OOBlitEngine
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
		../32blit
)
