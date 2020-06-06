include(${CMAKE_CURRENT_LIST_DIR}/32blit.cmake)

if (NOT DEFINED OOBLIT_ONCE)
	set(OOBLIT_ONCE TRUE)

	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/ooblit ooblit)
endif ()

function(blit_add_ooblit NAME)
	target_link_libraries(${NAME} OOBlitEngine)
endfunction()
