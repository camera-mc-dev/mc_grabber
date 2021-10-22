#include <iostream>
using std::cout;
using std::endl;

#include "mainWindow.h"

int main(int argc, char *argv[] )
{
	auto app = Gtk::Application::create(argc, argv, "recording session manager");
	
	MainWindow window;
	window.set_default_size(800, 800);
	
	app->run(window);
	
	
}
