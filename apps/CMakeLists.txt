add_executable(lobby
	lobby.cpp
	../src/game_client.cpp
	../src/scene_manager.cpp
)
target_link_libraries(lobby PRIVATE
	mscore
	lan
	widget
)
if(WIN32)
	# to make std::println work on windows
	# see https://stackoverflow.com/questions/78452682/stdprintln-print-not-working-in-winlibs-mingw-gcc-14-1
	target_link_libraries(lobby PRIVATE "-lstdc++exp")
endif()

# add_executable(game_minesweeper
# 	game_minesweeper.cpp
# )
# target_link_libraries(game_minesweeper PRIVATE
# 	minesweeper
# )

# add_executable(text_debug
# 	text_debug.cpp
# 	../src/scene_manager.cpp
# )
# target_link_libraries(text_debug PRIVATE
# 	SFML::Graphics
# 	widget
# )

# add_executable(packet_test
# 	packet_test.cpp
# )
# target_link_libraries(lan PUBLIC )
# target_link_libraries(packet_test PRIVATE
# 	widget
# 	lan
# )
