/*!
 * ResourceVisitor.h
 * Projekt 3DVisual
 */
#ifndef GPU_RESOURCEVISITOR_DEF
#define GPU_RESOURCEVISITOR_DEF 1

#include <vector>
#include <osgCompute/Resource>
#include <osgCompute/Visitor>

namespace Gpu
{
	/**
	*  \class NodePositions
	*  \brief Resource that stores position of nodes.
	*  \author Vladimir Polak
	*  \date 18. 3. 2012
	*/

	class NodePositions : public osgCompute::Resource
    {
    public:
        NodePositions() { _positions = new osg::Vec3Array; }

        META_Object(Gpu,NodePositions)

		osg::ref_ptr<osg::Vec3Array> _positions;

    protected:
        virtual ~NodePositions() {}

    private:
        NodePositions(const NodePositions&, const osg::CopyOp& ) {} 
        inline NodePositions &operator=(const NodePositions &) { return *this; }
    };

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

	protected:
		virtual ~ResourceVisitor() {clearLocal();}

    private:
		osg::ref_ptr<NodePositions> _positionsResource;

		void clearLocal();

        ResourceVisitor(const ResourceVisitor&, const osg::CopyOp& ) {} 
        inline ResourceVisitor &operator=(const ResourceVisitor &) { return *this; }
	};
	
}

#endif