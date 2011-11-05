#include "Core/Example.h"

Data::Graph* AppCore::Example::CreateCustomGraph()
{
	Data::Graph* example = new Data::Graph(1, "Graf", 0, 0, NULL);

	QMap<QString, QString> *settings = new QMap<QString, QString>;

	Data::Type* typ = new Data::Type(1, "Typ_1", example, settings);
	
	Data::Node* uzol1 = example->addNode(1, "Uzol_1", typ, Data::Vector(0,0,0));
	Data::Node* uzol2 = example->addNode(2, "Uzol_2", typ, Data::Vector(0,0,0));
	Data::Node* uzol3 = example->addNode(3, "Uzol_3", typ, Data::Vector(0,0,0));
	Data::Node* uzol4 = example->addNode(4, "Uzol_4", typ, Data::Vector(0,0,0));
	Data::Node* uzol5 = example->addNode(5, "Uzol_5", typ, Data::Vector(0,0,0));
	Data::Node* uzol6 = example->addNode(6, "Uzol_6", typ, Data::Vector(0,0,0));
	Data::Node* uzol7 = example->addNode(7, "Uzol_7", typ, Data::Vector(0,0,0));

	example->addEdge(1, "Hrana_1", uzol1, uzol2, typ, true);
	example->addEdge(2, "Hrana_2", uzol2, uzol3, typ, true);
	example->addEdge(3, "Hrana_3", uzol3, uzol1, typ, true);
	example->addEdge(4, "Hrana_4", uzol1, uzol5, typ, true);
	example->addEdge(5, "Hrana_5", uzol5, uzol6, typ, true);
	example->addEdge(6, "Hrana_6", uzol6, uzol7, typ, true);
	example->addEdge(7, "Hrana_7", uzol7, uzol3, typ, true);

	return example;

}