add_library(widget
	button.cpp
	splash_screen.cpp
)

target_link_libraries(widget PUBLIC
	SFML::Graphics
)

target_compile_features(widget PUBLIC cxx_std_17)

