/*!
 * LayoutModule.h
 * Projekt 3DVisual
 */
#ifndef GPU_LAYOUTMODULE_DEF
#define GPU_LAYOUTMODULE_DEF 1

#include <vector>
#include <osgCompute/Module>
#include <osgCompute/Memory>

namespace Gpu
{

	/**
	*  \class LayoutModule
	*  \brief Module attached to computation node responsible for computing node positions on GPU.
	*  \author Vladimir Polak
	*  \date 18. 3. 2012
	*/
	class LayoutModule : public osgCompute::Module 
    {
    public:

		/**
		*  \fn public constructor LayoutModule
		*  \brief Creates new LayoutModule object
		*/
        LayoutModule() : osgCompute::Module() {clearLocal();}
        
		META_Object( Gpu, LayoutModule );

		/**
         * \fn public virtual init
         * \brief Creates and initializes all resources necessary for the algorithm
		 * \return bool Returns true if module successfully initialized, false otherwise.
         */
        virtual bool init();

		/**
         * \fn public virtual launch
         * \brief Launches parallel layout algorithm
         */
        virtual void launch();

		/**
         * \fn public virtual acceptResource
         * \brief Exchanges resources required by parallel layout algorithm
		 * \param resource Reference to the resource
         */
        virtual void acceptResource( osgCompute::Resource& resource );

		/**
         * \fn inline public virtual clear
         * \brief Removes all resources of a module
         */
        virtual void clear() { clearLocal(); osgCompute::Module::clear(); }
		
		/**
         * \fn public initAlgorithmParameters
         * \brief Call host function to set parallel algorithm parameters
         */
		void initAlgorithmParameters();

    protected:
		
		/**
		*  \fn protected virtual destructor ~LayoutModule
		*  \brief Destroys the LayoutModule object
		*/
		virtual ~LayoutModule() { clearLocal(); }
        
		/**
         * \fn protected clearLocal
         * \brief Removes resources accepted by module
         */
		void clearLocal();
		
		/**
		*	osgCompute::Memory _vertexBuffer
		*	\brief Memory storage for node positions
		*/
		osg::ref_ptr<osgCompute::Memory>	_vertexBuffer;

		/**
		*	osgCompute::Memory _velocityBuffer
		*	\brief Memory storage for node velocities
		*/
		osg::ref_ptr<osgCompute::Memory>	_velocityBuffer;

		/**
		*	osgCompute::Memory _edgeBuffer
		*	\brief Memory storage for edges
		*/
		osg::ref_ptr<osgCompute::Memory>	_edgeBuffer;

    private:
        LayoutModule(const LayoutModule&, const osg::CopyOp& ) {} 
        inline LayoutModule &operator=(const LayoutModule &) { return *this; }
    };
	
}

#endif