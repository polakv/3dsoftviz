/*!
 * LayoutModule.h
 * Projekt 3DVisual
 */
#ifndef GPU_LAYOUTMODULE_DEF
#define GPU_LAYOUTMODULE_DEF 1

#include <vector>
#include <osgCompute/Module>
#include <osgCompute/Resource>
#include <osgCuda/Memory>

#include "Gpu/ResourceVisitor.h"

namespace Gpu
{

	/**
	*  \class LayoutModule
	*  \brief Module attached to computation node that computes node positions on gpu.
	*  \author Vladimir Polak
	*  \date 18. 3. 2012
	*/
	class LayoutModule : public osgCompute::Module 
    {
    public:
        LayoutModule() {clearLocal();}

        META_Object( Gpu, LayoutModule )

        // Modules have to implement at least this three methods:
        virtual bool init();
        virtual void launch();
        virtual void acceptResource( osgCompute::Resource& resource );

        virtual void clear() { clearLocal(); osgCompute::Module::clear(); }
    protected:
		virtual ~LayoutModule() { clearLocal(); }
        void clearLocal();

		unsigned int                        _numBlocks;
        unsigned int                        _numThreads;

		osg::ref_ptr<osgCompute::Memory>		_vertexBuffer;
		osg::ref_ptr<osgCompute::Memory>		_edgeIndexBuffer;
		osg::ref_ptr<osgCompute::Memory>		_edgeValueBuffer;

    private:
        LayoutModule(const LayoutModule&, const osg::CopyOp& ) {} 
        inline LayoutModule &operator=(const LayoutModule &) { return *this; }
    };
	
}

#endif