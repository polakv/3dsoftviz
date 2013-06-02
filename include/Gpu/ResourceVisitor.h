/*!
 * ResourceVisitor.h
 * Projekt 3DVisual
 */
#ifdef HAVE_CUDA

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
	*  \brief Custom resource visitor that collects required node data from Node objects and distributes them to compute nodes.
	*  \author Vladimir Polak
	*  \date 18. 3. 2012
	*/
	class ResourceVisitor : public osgCompute::ResourceVisitor
	{
	public:
		/**
		*  \fn public constructor ResourceVisitor(bool randomize)
		*  \brief Creates new ResourceVisitor object.
		*  \param randomize If true, randomize node positions during traversal.
		*/
		ResourceVisitor(bool randomize = false);
	  
		/**
		*  \fn public virtual collect
		*  \brief Count graph nodes, stores their offsets and collects adjanced edges.
		*  \param node a reference to the current node
		*/
		virtual void collect( osg::Node& node );

		/**
		*  \fn public virtual distribute
		*  \brief Add resources and update Node objects pointers to shared memory with GPU, also distributes resources to compute nodes.
		*  \param node a reference to the current node
		*/
		virtual void distribute( osg::Node& node );

		/**
		*  \fn public  exchange
		*  \brief Calls parent implementation.
		*  \param node a reference to the current node
		*/
		virtual void exchange( osg::Node& node );

	protected:
		/**
		*  \fn protected virtual destructor ~ResourceVisitor
		*  \brief Destroys the ResourceVisitor object
		*/
		virtual ~ResourceVisitor() {clearLocal();}	

    private:
		/**
         * \fn private addPositionResource
         * \brief Creates memory objects from collected data and adds them as resources for further distribution to compute nodes.
         */
		void addPositionResource();

		/**
         * \fn private getRandomPosition
         * \brief Generates random position for node.
		 * \return osg::Vec3f random position
         */
		osg::Vec3f getRandomPosition();

		/**
         * \fn private getRandomDouble
         * \brief Generates random double .
		 * \return double random double value
         */
		double getRandomDouble();

		/**
         * \fn private clearLocal
         * \brief Reset used attributes.
         */
		void clearLocal();
		
		/**
		*	unsigned int _nodeCount
		*	\brief Count of graph nodes.
		*/
		unsigned int _nodeCount;

		/**
		*	QMap<qlonglong, unsigned int>* _vertexOffsets
		*	\brief Memory storage for nodes.
		*/
		QMap<qlonglong, unsigned int>* _vertexOffsets;

		/**
		*	QMap<qlonglong, osg::ref_ptr<Data::Edge> >* _edges
		*	\brief Map of collected edges.
		*/
        QMap<qlonglong, osg::ref_ptr<Data::Edge> >*	_edges;

		/**
		*	osgCompute::Memory _vertexBuffer
		*	\brief Memory storage for nodes.
		*/
		osg::ref_ptr<osgCompute::Memory> _vertexBuffer;

		/**
		*	bool _randomize
		*	\brief Radomize node positions during travesal.
		*/
		bool _randomize;

        ResourceVisitor(const ResourceVisitor&, const osg::CopyOp& ) {} 
        inline ResourceVisitor &operator=(const ResourceVisitor &) { return *this; }
	};
	
}

#endif //GPU_RESOURCEVISITOR_DEF
#endif //HAVE_CUDA
