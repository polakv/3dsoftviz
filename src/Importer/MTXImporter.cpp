#include "Importer/MTXImporter.h"
#include "Importer/GraphOperations.h"
#include "Importer/ReadNodesStore.h"

#include <string>
#include <memory>
namespace Importer {

bool MTXImporter::import (
	ImporterContext &context
) {
	QTextStream stream (&(context.getStream ()));
	QString line, graphname="Graph";

	GraphOperations graphOp (context.getGraph ());
	ReadNodesStore readNodes;
	bool edgeOrientedDefault = false;
	bool edgeOrientedDefaultForce = false;
	Data::Type *edgeType = NULL;
	Data::Type *nodeType = NULL;
	context.getGraph ().setName (graphname);
	(void)graphOp.addDefaultTypes (edgeType, nodeType);

	//preskocenie hlavicky
	do {
		line = stream.readLine();
	} while ( !stream.atEnd() && line.startsWith(QString("%")) );
	//v premenej line sa nachadzaju udaje o matici

	int i = 0;
	
	//citanie vstupneho suboru
	while ( !stream.atEnd() ) {
		line = stream.readLine();
		QStringList words;
		words = line.split (QRegExp (QString ("[ \t]+")));
		int size = words.size ();
		Data::Type *newNodeType = NULL;
		osg::ref_ptr<Data::Node> node1, node2;
		osg::ref_ptr<Data::Edge> edge;
		if (size < 2){
			printf("%d",size);
			context.getInfoHandler ().reportError ("Zvoleny subor nie je validny Matrix Market subor.");
			return false;
		}
		else{
			QString srcNodeName = words[0];
			QString dstNodeName = words[1];

			// vytvorenie pociatocneho uzla
			if (!readNodes.contains(srcNodeName))
			{
				node1 = context.getGraph().addNode(srcNodeName, nodeType);
				readNodes.addNode(srcNodeName, node1);
			} else {
				node1=readNodes.get(srcNodeName);
			}

			//vytvorenie koncoveho uzla
			if (!readNodes.contains(dstNodeName))
			{
				node2 = context.getGraph().addNode(dstNodeName, nodeType);
				readNodes.addNode(dstNodeName, node2);
			} else {
				node2=readNodes.get(dstNodeName);
			}
			//vytvoreniez hrany
			QString edgeName = srcNodeName + dstNodeName;
			edge = context.getGraph().addEdge(
				edgeName,
				node1,
				node2,
				edgeType,
				edgeOrientedDefault
			);
		}

	}

	return true;
}

} // namespace
