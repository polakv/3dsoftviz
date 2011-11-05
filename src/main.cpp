#include <QApplication>

#include "Core/Core.h"

int main(int argc, char *argv[])
{     
	QApplication app(argc, argv);
	//new Cleaner(&app);
        AppCore::Core::getInstance(&app);
        //Manager::GraphManager::getInstance();

}


