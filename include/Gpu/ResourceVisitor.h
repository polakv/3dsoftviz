/*!
 * ResourceVisitor.h
 * Projekt 3DVisual
 */
#ifndef GPU_RESOURCEVISITOR_DEF
#define GPU_RESOURCEVISITOR_DEF 1

#include <vector>
#include <QMap>
#include <osgCompute/Visitor>
#include <osgCompute/Memory>
#include "Data/Node.h"
#include "Data/Edge.h"

namespace Gpu
{
	/**
	*  \class ResourceVisitor
	*  \brief Custom resource visitor that collects node positions from transform nodes.
	*  \author Vladimir Polak
	*  \date 18. 3. 2012
	*/
	class ResourceVisitor : public osgCompute::ResourceVisitor
	{
	public:

		ResourceVisitor();
	  
		/**
			*  \fn public  collect(osg::Node& node)
			*  \brief Collect node position resources from AutoTransform nodes
			*  \param node a reference to the current node
			*/
		virtual void collect( osg::Node& node );
		virtual void distribute( osg::Node& node );
		virtual void exchange( osg::Node& node );

	protected:
		virtual ~ResourceVisitor() {clearLocal();}

    private:
		void clearLocal();
		void addPositionResource();
		osg::Vec3f getRandomLocation();
		double getRandomDouble();

		unsigned int								_nodeCount;
		osg::ref_ptr<osgCompute::Memory>			_vertexBuffer;
		QMap<qlonglong, unsigned int>*				_vertexOffsets;
		QMap<qlonglong, osg::ref_ptr<Data::Edge>>*	_edges;

        ResourceVisitor(const ResourceVisitor&, const osg::CopyOp& ) {} 
        inline ResourceVisitor &operator=(const ResourceVisitor &) { return *this; }
	};
	
}

#endif