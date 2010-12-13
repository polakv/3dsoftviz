#include "QOSG/LoadGraphWindow.h"

using namespace QOSG;

LoadGraphWindow::LoadGraphWindow(QWidget *parent)
{
	setModal(true);
	resize(600,250);	 
    setWindowTitle(tr("Load graph from database"));

	loadButton = createButton(tr("&Load"), SLOT(loadGraph()));

	QPushButton *cancelButton = new QPushButton(tr("Cancel"));
	cancelButton->setFocusPolicy(Qt::NoFocus);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(close()));

	numberOfGraphs = new QLabel;

	graphList<<tr("ID")<<tr("Name")<<tr("Number of nodes")<<tr("Number of edges");
	graphsTable=new QTableWidget(this);
	graphsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	graphsTable->setRowCount(0);
	graphsTable->setColumnCount(4);
	graphsTable->setHorizontalHeaderLabels(graphList);
	graphsTable->horizontalHeader()->setResizeMode(0, QHeaderView::ResizeToContents);
	graphsTable->horizontalHeader()->setResizeMode(1, QHeaderView::Stretch);
	graphsTable->horizontalHeader()->setResizeMode(2, QHeaderView::ResizeToContents);
	graphsTable->horizontalHeader()->setResizeMode(3, QHeaderView::ResizeToContents);
	graphsTable->verticalHeader()->hide();
    graphsTable->setShowGrid(true);

	createGraphTable();

	QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(loadButton);
	buttonsLayout->addWidget(cancelButton);

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(graphsTable, 3, 0, 1, 3);
	mainLayout->addWidget(numberOfGraphs, 4, 0, 1, 3);
    mainLayout->addLayout(buttonsLayout, 5, 0, 1, 3);
    setLayout(mainLayout);
}

void LoadGraphWindow::createGraphTable()
{
	Manager::GraphManager * manager = Manager::GraphManager::getInstance();
	Model::DB * db = manager->getDB();
	bool error = false;
	qlonglong id, graphsCount, row;
	QList<qlonglong> nodes;
	QList<qlonglong> edges;
	QString name;
	QMap<qlonglong, Data::Graph*>::iterator iterGraph;
	
	QMap<qlonglong, Data::Graph *> graphs = Model::GraphDAO::getGraphs(db->tmpGetConn(), &error);
	graphsCount = graphs.count(); 

	numberOfGraphs->setText(tr("%1 graph(s) found").arg(graphsCount));
	graphsTable->setRowCount(graphsCount);
	nodes = Model::NodeDAO::getListOfNodes(db->tmpGetConn(), &error);
	edges = Model::EdgeDAO::getListOfEdges(db->tmpGetConn(), &error);

	qDebug() << "[QOSG::LoadGraphWindow::createGraphTable] total number of nodes in DB: " << nodes.count();
	qDebug() << "[QOSG::LoadGraphWindow::createGraphTable] total number of edges in DB: " << edges.count();
	
	for(iterGraph = graphs.begin(), row=0; iterGraph != graphs.end(); ++iterGraph, row++)
	{
		id = iterGraph.key();
		
		name = graphs.value(id)->getName();

		QTableWidgetItem *itemID = new QTableWidgetItem(tr("%1").arg(id));
		QTableWidgetItem *itemName = new QTableWidgetItem(name);
		QTableWidgetItem *itemNumberOfNodes = new QTableWidgetItem(tr("%1").arg(nodes.count(id)));
		QTableWidgetItem *itemNumberOfEdges = new QTableWidgetItem(tr("%1").arg(edges.count(id)));
		graphsTable->setItem(row, 0, itemID);
		graphsTable->setItem(row, 1, itemName);
		graphsTable->setItem(row, 2, itemNumberOfNodes);
		graphsTable->setItem(row, 3, itemNumberOfEdges);
	}
	if(graphsTable->rowCount() > 0)
		graphsTable->selectRow(0);
}

void LoadGraphWindow::loadGraph()
{
	qlonglong graphID;
	qlonglong layoutID;

	//plati len ak sa grafy cisluju postupne od 2 - treba to este opravit
	graphID = graphsTable->currentRow() + 2;

	//plati ak pracujeme len s jednym layoutom - tiez treba este opravit ked budeme chciet pracovat s viac layoutmi
	layoutID = graphID;

	qDebug() << "[QOSG::LoadGraphWindow::loadGraph] Selected graph ID: " << graphID;

	if(graphsTable->rowCount() > 0) 
	{
		Manager::GraphManager * manager = Manager::GraphManager::getInstance();
		Model::DB * db = manager->getDB();
		bool error = false;

		manager->loadGraphFromDB(graphID, layoutID);
		this->close();
	}
	else 
	{
		qDebug() << "[QOSG::LoadGraphWindow::loadGraph] There are no graphs saved in DB.";
	}
}

QPushButton *LoadGraphWindow::createButton(const QString &text, const char *member)
{
    QPushButton *button = new QPushButton(text);
    connect(button, SIGNAL(clicked()), this, member);
    return button;
}