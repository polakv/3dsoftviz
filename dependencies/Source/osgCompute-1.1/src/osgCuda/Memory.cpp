#include <memory.h>
#if defined(__linux)
#include <malloc.h>
#endif
#include <cuda_runtime.h>
#include <driver_types.h>
#include <osg/Notify>
#include <osgCuda/Memory>

namespace osgCuda
{
	/**
    */
    class MemoryObject : public osgCompute::MemoryObject
    {
    public:
        void*							_devPtr;
        cudaArray*                      _devArray;
        void*							_hostPtr;
        unsigned int                    _modifyCount;

        MemoryObject();
        virtual ~MemoryObject();

    private:
        // not allowed to call copy-constructor or copy-operator
        MemoryObject( const MemoryObject& ) {}
        MemoryObject& operator=( const MemoryObject& ) { return *this; }
    };

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // PUBLIC FUNCTIONS /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //------------------------------------------------------------------------------
    MemoryObject::MemoryObject()
        :   osgCompute::MemoryObject(),
        _devPtr(NULL),
        _devArray(NULL),
        _hostPtr(NULL),
        _modifyCount(UINT_MAX)
    {
    }

    //------------------------------------------------------------------------------
    MemoryObject::~MemoryObject()
    {
        if( NULL != _devPtr)
        {
            cudaError res = cudaFree( _devPtr );
            if( res != cudaSuccess )
            {
                osg::notify(osg::FATAL)
                    <<"MemoryObject::~MemoryObject(): error during cudaFree(). "
                    <<cudaGetErrorString(res)<<std::endl;
            }
        }

        if( NULL != _devArray )
        {
            cudaError_t res = cudaFreeArray( _devArray );
            if( res != cudaSuccess )
            {
                osg::notify(osg::FATAL)
                    <<"MemoryObject::~MemoryObject(): error during cudaFreeArray(). "
                    <<cudaGetErrorString(res)<<std::endl;
            }
        }

        if( NULL != _hostPtr)
            free( _hostPtr );
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////
    // PUBLIC FUNCTIONS /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //------------------------------------------------------------------------------
    Memory::Memory()
        : osgCompute::Memory()
    {
        clearLocal();
    }

    //------------------------------------------------------------------------------
    void Memory::clear()
    {
        clearLocal();
        osgCompute::Memory::clear();
    }

    //------------------------------------------------------------------------------
    bool Memory::init()
    {
        unsigned int numElements = 1;
        for( unsigned int d=0; d<getNumDimensions(); ++d )
            numElements *= getDimension( d );

        unsigned int byteSize = numElements * getElementSize();

        if( getElementSize() == 0 || getNumDimensions() == 0 )
        {
            osg::notify(osg::WARN)
                << getName() 
                << " [osgCuda::Memory::init()] \""<<getName()<<"\": "
                << "no valid buffer size. Call setElementSize() and setDimension() first."
                << std::endl;

                clear();
                return false;
        }

        // Check image data
        if( _image.valid() )
        {
            if( _image->getNumMipmapLevels() > 1 )
            {
                osg::notify(osg::WARN)
                    << getName() << " [osgCuda::Memory::init()] \""<<getName()<<"\": Image \""
                    << _image->getName() << "\" uses MipMaps which are currently"
                    << "not supported."
                    << std::endl;

                clear();
                return false;
            }

            if( _image->getTotalSizeInBytes() != byteSize )
            {
                osg::notify(osg::WARN)
                    << getName() << " [osgCuda::Memory::init()] \""<<getName()<<"\": size of image \""
                    << _image->getName() << "\" is wrong."
                    << std::endl;

                clear();
                return false;
            }
        }

        // The ChannelFormatDescription should be checked by the application

        return osgCompute::Memory::init();
    }

    //------------------------------------------------------------------------------
    void* Memory::map( unsigned int mapping/* = osgCompute::MAP_DEVICE*/, unsigned int offset/* = 0*/, unsigned int hint )
    {
        if( osgCompute::Resource::isClear() )
            if( !init() )
                return NULL;

        if( mapping == osgCompute::UNMAP )
        {
            unmap( hint );
            return NULL;
        }

        ////////////////////
        // RECEIVE HANDLE //
        ////////////////////
        MemoryObject* memoryPtr = dynamic_cast<MemoryObject*>( object() );
        if( !memoryPtr )
            return NULL;
        MemoryObject& memory = *memoryPtr;

        /////////////////////////////
        // CHECK FOR MODIFICATIONS //
        /////////////////////////////
        bool firstLoad = false;
        // image or array has changed
        bool needsSetup = false;
        if( (_image.valid() && _image->getModifiedCount() != memory._modifyCount ) )
            needsSetup = true;

        // current mapping
        memory._mapping = mapping;

        //////////////
        // MAP DATA //
        //////////////
        void* ptr = NULL;
        if( (memory._mapping & osgCompute::MAP_HOST) )
        {
            /////////////////////
            // ALLOCATE STREAM //
            /////////////////////
            if( NULL == memory._hostPtr )
            {
                // allocate host-memory 
                if( !alloc( mapping ) )
                    return NULL;

                firstLoad = true;
            }

            //////////////////
            // SETUP STREAM //
            //////////////////
            if( needsSetup )
                if( !setup( mapping ) )
                    return NULL;

            /////////////////
            // SYNC STREAM //
            /////////////////
            if( (memory._syncOp & osgCompute::SYNC_HOST) )
                if( !sync( mapping ) )
                    return NULL;

            ptr = memory._hostPtr;
        }
        else if( (mapping & osgCompute::MAP_DEVICE_ARRAY) == osgCompute::MAP_DEVICE_ARRAY )
        {
            /////////////////////
            // ALLOCATE STREAM //
            /////////////////////
            if( NULL == memory._devArray )
            {
                // allocate device-memory 
                if( !alloc( mapping ) )
                    return NULL;

                firstLoad = true;
            }

            //////////////////
            // SETUP STREAM //
            //////////////////
            if( needsSetup )
                if( !setup( mapping ) )
                    return NULL;

            /////////////////
            // SYNC STREAM //
            /////////////////
            if( memory._syncOp & osgCompute::SYNC_ARRAY )
                if( !sync( mapping ) )
                    return NULL;

            ptr = memory._devArray;
        }

        else if( (memory._mapping & osgCompute::MAP_DEVICE) )
        {
            /////////////////////
            // ALLOCATE STREAM //
            /////////////////////
            if( NULL == memory._devPtr )
            {
                // allocate device-memory 
                if( !alloc( mapping ) )
                    return NULL;

                firstLoad = true;
            }

            //////////////////
            // SETUP STREAM //
            //////////////////
            if( needsSetup && !(memory._syncOp & osgCompute::SYNC_DEVICE) )
                if( !setup( mapping ) )
                    return NULL;

            /////////////////
            // SYNC STREAM //
            /////////////////
            if( (memory._syncOp & osgCompute::SYNC_DEVICE) )
                if( !sync( mapping ) )
                    return NULL;

            ptr = memory._devPtr;
        }
        else
        {
            osg::notify(osg::WARN)
                << getName() << " [osgCuda::Memory::map()] \""<<getName()<<"\": Wrong mapping. Use one of the following: "
                << "HOST_SOURCE, HOST_TARGET, HOST, DEVICE_SOURCE, DEVICE_TARGET,  DEVICE_ARRAY, DEVICE."
                << std::endl;

            return NULL;
        }

        // return if something failed
        if( NULL ==  ptr )
            return NULL; 

        //////////////////
        // LOAD/SUBLOAD //
        //////////////////
        if( getSubloadCallback() )
        {
            const osgCompute::SubloadCallback* callback = getSubloadCallback();
            if( callback )
            {
                // load or subload data before returning the pointer
                if( firstLoad )
                    callback->load( ptr, mapping, offset, *this );
                else
                    callback->subload( ptr, mapping, offset, *this );
            }
        }

        // check sync
        if( (mapping & osgCompute::MAP_DEVICE_ARRAY_TARGET) == osgCompute::MAP_DEVICE_ARRAY_TARGET )
        {
            memory._syncOp |= osgCompute::SYNC_DEVICE;
            memory._syncOp |= osgCompute::SYNC_HOST;
        }
        else if( (mapping & osgCompute::MAP_DEVICE_TARGET) == osgCompute::MAP_DEVICE_TARGET )
        {
            memory._syncOp |= osgCompute::SYNC_ARRAY;
            memory._syncOp |= osgCompute::SYNC_HOST;
        }
        else if( (mapping & osgCompute::MAP_HOST_TARGET) == osgCompute::MAP_HOST_TARGET )
        {
            memory._syncOp |= osgCompute::SYNC_ARRAY;
            memory._syncOp |= osgCompute::SYNC_DEVICE;
        }

        return &static_cast<char*>(ptr)[offset];
    }

    //------------------------------------------------------------------------------
    void Memory::unmap( unsigned int )
    {
        if( osgCompute::Resource::isClear() )
            if( !init() )
                return;

        ////////////////////
        // RECEIVE HANDLE //
        ////////////////////
        MemoryObject* memoryPtr = dynamic_cast<MemoryObject*>( object() );
        if( !memoryPtr )
            return;
        MemoryObject& memory = *memoryPtr;

        ////////////////
        // SETUP FLAG //
        ////////////////
        memory._mapping = osgCompute::UNMAP;
    }

    //------------------------------------------------------------------------------
    bool Memory::reset( unsigned int )
    {
        if( osgCompute::Resource::isClear() )
            if( !init() )
                return false;

        ////////////////////
        // RECEIVE HANDLE //
        ////////////////////
        MemoryObject* memoryPtr = dynamic_cast<MemoryObject*>( object() );
        if( !memoryPtr )
            return false;
        MemoryObject& memory = *memoryPtr;

        // reset memory from array/image data 
        // during next call of map()
        memory._modifyCount = UINT_MAX;
        memory._syncOp = osgCompute::NO_SYNC;

        // clear host memory
        if( memory._hostPtr != NULL )
        {
            if( !memset( memory._hostPtr, 0x0, getByteSize() ) )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::reset()] \"" << getName() << "\": error during memset() for host memory."
                    << std::endl;

                return false;
            }
        }

        // clear device memory
        if( memory._devPtr != NULL )
        {
            cudaError res;
            if( getNumDimensions() == 3 )
            {
                cudaPitchedPtr pitchedPtr = make_cudaPitchedPtr( memory._devPtr, memory._pitch, getDimension(0)*getElementSize(), getDimension(1) );
                cudaExtent extent = make_cudaExtent( getPitch(), getDimension(1), getDimension(2) );
                res = cudaMemset3D( pitchedPtr, 0x0, extent );
                if( res != cudaSuccess )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::reset()] \"" << getName() << "\": error during cudaMemset3D() for device memory."
                        << cudaGetErrorString( res )  <<"."
                        << std::endl;

                    unmap();
                    return false;
                }
            }
            else if( getNumDimensions() == 2 )
            {
                res = cudaMemset2D( memory._devPtr, memory._pitch, 0x0, getDimension(0)*getElementSize(), getDimension(1) );
                if( res != cudaSuccess )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::reset()] \"" << getName() << "\": error during cudaMemset2D() for device memory."
                        << cudaGetErrorString( res )  <<"."
                        << std::endl;

                    unmap();
                    return false;
                }
            }
            else
            {
                res = cudaMemset( memory._devPtr, 0x0, getByteSize() );
                if( res != cudaSuccess )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::reset()] \"" << getName() << "\": error during cudaMemset() for device memory."
                        << cudaGetErrorString( res )  <<"."
                        << std::endl;

                    unmap();
                    return false;
                }
            }
        }

        return true;
    }

    //------------------------------------------------------------------------------
    bool Memory::supportsMapping( unsigned int mapping, unsigned int ) const
    {
        switch( mapping )
        {
        case osgCompute::UNMAP:
        case osgCompute::MAP_HOST:
        case osgCompute::MAP_HOST_SOURCE:
        case osgCompute::MAP_HOST_TARGET:
        case osgCompute::MAP_DEVICE:
        case osgCompute::MAP_DEVICE_SOURCE:
        case osgCompute::MAP_DEVICE_TARGET:
        case osgCompute::MAP_DEVICE_ARRAY:
            return true;
        default:
            return false;
        }
    }

    //------------------------------------------------------------------------------
    bool Memory::setup( unsigned int mapping )
    {
        cudaError res;

        ////////////////////
        // RECEIVE HANDLE //
        ////////////////////
        MemoryObject* memoryPtr = dynamic_cast<MemoryObject*>( object() );
        if( !memoryPtr )
            return false;
        MemoryObject& memory = *memoryPtr;

        //////////////////
        // SETUP MEMORY //
        //////////////////
        if( (mapping & osgCompute::MAP_DEVICE_ARRAY) == osgCompute::MAP_DEVICE_ARRAY )
        {
            const void* data = NULL;
            if( _image.valid() )
            {
                data = _image->data();
            }

            if( data == NULL )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": Cannot receive valid data pointer."
                    << std::endl;

                return false;
            }

            if( getNumDimensions() == 3 )
            {
                cudaMemcpy3DParms memCpyParams = {0};
                memCpyParams.dstArray = memory._devArray;
                memCpyParams.kind = cudaMemcpyHostToDevice;
                memCpyParams.srcPtr = make_cudaPitchedPtr((void*)data, getDimension(0)*getElementSize(), getDimension(0), getDimension(1));

                cudaExtent arrayExtent = {0};
                arrayExtent.width = getDimension(0);
                arrayExtent.height = getDimension(1);
                arrayExtent.depth = getDimension(2);

                memCpyParams.extent = arrayExtent;

                res = cudaMemcpy3D( &memCpyParams );
                if( cudaSuccess != res )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": cudaMemcpy3D() failed."
                        << " " << cudaGetErrorString( res ) <<"."
                        << std::endl;

                    return false;
                }
            }
            else if( getNumDimensions() == 2 )
            {
                cudaError res = cudaMemcpy2DToArray( memory._devArray, 0, 0, data, getDimension(0)*getElementSize(), getDimension(0)*getElementSize(), getDimension(1), cudaMemcpyHostToDevice );  
                if( cudaSuccess != res )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::set()] \""<<getName()<<"\": cudaMemcpy2DToArray() failed."
                        << " " << cudaGetErrorString( res ) << "."
                        << std::endl;

                    return false;
                }
            }
            else
            {
                res = cudaMemcpyToArray(memory._devArray, 0, 0, data, getByteSize(), cudaMemcpyHostToDevice);
                if( cudaSuccess != res )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": cudaMemcpyToArray() failed."
                        << " " << cudaGetErrorString( res ) << "."
                        << std::endl;

                    return false;
                }
            }

            if( (memory._syncOp & osgCompute::SYNC_ARRAY) == osgCompute::SYNC_ARRAY )
                memory._syncOp ^= osgCompute::SYNC_ARRAY;

            // host must be synchronized
            // because device memory has been modified
            memory._syncOp |= osgCompute::SYNC_DEVICE;
            memory._syncOp |= osgCompute::SYNC_HOST;

            memory._modifyCount = _image.valid()? _image->getModifiedCount() : UINT_MAX;
            return true;
        }
        else if( mapping & osgCompute::MAP_DEVICE )
        {
            void* data = NULL;
            if( _image.valid() )
            {
                data = _image->data();
            }

            if( data == NULL )
            {
                osg::notify(osg::WARN)
                    << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": cannot receive valid data pointer."
                    << std::endl;

                return false;
            }


            if( getNumDimensions() == 3 )
            {
                cudaMemcpy3DParms memcpyParams = {0};
                memcpyParams.dstPtr = make_cudaPitchedPtr( memory._devPtr, memory._pitch, getDimension(0), getDimension(1) );
                memcpyParams.srcPtr = make_cudaPitchedPtr( data, getDimension(0) * getElementSize(), getDimension(0), getDimension(1) );
                memcpyParams.extent = make_cudaExtent( getDimension(0) * getElementSize(), getDimension(1), getDimension(2) );
                memcpyParams.kind = cudaMemcpyHostToDevice;

                res = cudaMemcpy3D( &memcpyParams );
                if( cudaSuccess != res )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": error during cudaMemcpy3D()."
                        << " " << cudaGetErrorString( res ) <<"."
                        << std::endl;

                    return false;
                }
            }
            else if( getNumDimensions() == 2 )
            {
                res = cudaMemcpy2D( memory._devPtr, memory._pitch, data, getDimension(0) * getElementSize(), getDimension(0), getDimension(1), cudaMemcpyHostToDevice );
                if( cudaSuccess != res )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": error during cudaMemcpy2D()."
                        << " " << cudaGetErrorString( res ) <<"."
                        << std::endl;

                    return false;
                } 
            }
            else
            {
                res = cudaMemcpy( memory._devPtr,  data, getByteSize(), cudaMemcpyHostToDevice );
                if( cudaSuccess != res )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": error during cudaMemcpy()."
                        << " " << cudaGetErrorString( res ) <<"."
                        << std::endl;

                    return false;
                }
            }

            if( (memory._syncOp & osgCompute::SYNC_DEVICE) == osgCompute::SYNC_DEVICE )
                memory._syncOp ^= osgCompute::SYNC_DEVICE;

            // host must be synchronized
            // because device memory has been modified
            memory._syncOp |= osgCompute::SYNC_ARRAY;
            memory._syncOp |= osgCompute::SYNC_HOST;

            memory._modifyCount = _image.valid()? _image->getModifiedCount() : UINT_MAX;
           
            return true;
        }
        else if( mapping & osgCompute::MAP_HOST )
        {
            const void* data = NULL;
            if( _image.valid() )
            {
                data = _image->data();
            }

            if( data == NULL )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": cannot receive valid data pointer."
                    << std::endl;

                return false;
            }

            res = cudaMemcpy( memory._hostPtr,  data, getByteSize(), cudaMemcpyHostToHost );
            if( cudaSuccess != res )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::setup()] \""<<getName()<<"\": error during cudaMemcpy()."
                    << " " << cudaGetErrorString( res ) <<"."
                    << std::endl;

                return false;
            }

            if( (memory._syncOp & osgCompute::SYNC_HOST) == osgCompute::SYNC_HOST )
                memory._syncOp ^= osgCompute::SYNC_HOST;

            // Device must be synchronized
            // because host memory has been modified
            memory._syncOp |= osgCompute::SYNC_ARRAY;
            memory._syncOp |= osgCompute::SYNC_DEVICE;

            memory._modifyCount = _image.valid()? _image->getModifiedCount() : UINT_MAX;
           
            return true;
        }

        return false;
    }

    //------------------------------------------------------------------------------
    bool Memory::alloc( unsigned int mapping )
    {
        ////////////////////
        // RECEIVE HANDLE //
        ////////////////////
        MemoryObject* memoryPtr = dynamic_cast<MemoryObject*>( object() );
        if( !memoryPtr )
            return false;
        MemoryObject& memory = *memoryPtr;

        //////////////////
        // ALLOC MEMORY //
        //////////////////
        if( mapping & osgCompute::MAP_HOST )
        {
            if( memory._hostPtr != NULL )
                return true;

            memory._hostPtr = malloc( getByteSize() );
            if( NULL == memory._hostPtr )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::alloc()] \""<<getName()<<"\": error during malloc()."
                    << std::endl;

                return false;
            }

            // clear memory
            memset( memory._hostPtr, 0x0, getByteSize() );

            if( memory._devPtr != NULL || memory._devArray != NULL )
                memory._syncOp |= osgCompute::SYNC_HOST;

            return true;
        }
        else if( (mapping & osgCompute::MAP_DEVICE_ARRAY) == osgCompute::MAP_DEVICE_ARRAY )
        {
            if( memory._devArray != NULL )
                return true;

            const cudaChannelFormatDesc& desc = getChannelFormatDesc();
            if( (desc.x == 0 && desc.y == 0 && desc.z == 0 && desc.w == 0) || desc.f == cudaChannelFormatKindNone )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::allocStream()] \""<<getName()<<"\": no valid ChannelFormatDesc found."
                    << std::endl;

                return false;
            }

            if( getNumDimensions() == 3 )
            {
                cudaExtent extent;
                extent.width = getDimension(0);
                extent.height = (getDimension(1) <= 1)? 0 : getDimension(1);
                extent.depth = (getDimension(2) <= 1)? 0 : getDimension(2);

                // allocate memory
                cudaError res = cudaMalloc3DArray( &memory._devArray, &desc, extent );
                if( cudaSuccess != res || NULL ==  memory._devArray )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::allocStream()] \""<<getName()<<"\": something goes wrong within cudaMalloc3DArray()."
                        << " " << cudaGetErrorString( res ) <<"."
                        << std::endl;

                    return false;
                }
            }
            else if( getNumDimensions() == 2 )
            {
                cudaError res = cudaMallocArray( &memory._devArray, &desc, getDimension(0), (getDimension(1) <= 1)? 0 : getDimension(1) );
                if( cudaSuccess != res || NULL ==  memory._devArray )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::allocStream()] \""<<getName()<<"\": something goes wrong within mallocDevice2DArray()."
                        << " " << cudaGetErrorString( res ) <<"."
                        << std::endl;

                    return false;
                }
            }
            else
            {
                cudaError res = cudaMallocArray( &memory._devArray, &desc, getDimension(0), 1 );
                if( cudaSuccess != res || NULL ==  memory._devArray )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::allocStream()] \""<<getName()<<"\": something goes wrong within mallocDeviceArray()."
                        << " " << cudaGetErrorString( res ) <<"."
                        << std::endl;

                    return false;
                }
            }

            if( memory._hostPtr != NULL || memory._devPtr != NULL )
                memory._syncOp |= osgCompute::SYNC_ARRAY;

            return true;
        }
        else if( mapping & osgCompute::MAP_DEVICE )
        {
            if( memory._devPtr != NULL )
                return true;

            if( getNumDimensions() == 3 )
            {
                cudaPitchedPtr pitchPtr;
                cudaExtent extent;
                extent.width = getDimension(0) * getElementSize();
                extent.height = getDimension(1);
                extent.depth = getDimension(2);

                // allocate memory
                cudaError_t res = cudaMalloc3D( &pitchPtr, extent );
                if( cudaSuccess != res || NULL == pitchPtr.ptr )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::alloc()] \""<<getName()<<"\": error during mallocDevice3D()."
                        << cudaGetErrorString( res )  <<"."
                        << std::endl;

                    return false;
                }

                memory._devPtr = pitchPtr.ptr;
                memory._pitch = pitchPtr.pitch;

                // clear memory
                cudaMemset3D( pitchPtr, 0x0, extent );
            }
            else if( getNumDimensions() == 2 )
            {
                cudaError_t res = cudaMallocPitch( &memory._devPtr, (size_t*)&memory._pitch, getDimension(0) * getElementSize(), getDimension(1) );
                if( cudaSuccess != res )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::alloc()] \""<<getName()<<"\": error during mallocDevice2D()."
                        << cudaGetErrorString( res )  <<"."
                        << std::endl;

                    return false;
                }


                // clear memory
                cudaMemset2D( memory._devPtr, memory._pitch, 0x0, getDimension(0), getDimension(1) );
            }
            else
            {
                cudaError_t res = cudaMalloc( &memory._devPtr, getByteSize() );
                if( res != cudaSuccess )
                {
                    osg::notify(osg::FATAL)
                        << getName() << " [osgCuda::Memory::alloc()] \""<<getName()<<"\": error during mallocDevice()."
                        << cudaGetErrorString( res )  <<"."
                        << std::endl;

                    return false;
                }

                memory._pitch = getByteSize();
                // clear memory
                cudaMemset( memory._devPtr, 0x0, getByteSize() );
            }

            if( memory._pitch != (getDimension(0) * getElementSize()) )
            {
                int device = 0;
                cudaGetDevice( &device );
                cudaDeviceProp devProp;
                cudaGetDeviceProperties( &devProp, device );

                osg::notify(osg::INFO)
                    << getName() << " [osgCuda::Memory::alloc()] \""<<getName()
                    << "\": Memory requirement is not a multiple of texture alignment. This "
                    << "leads to a pitch which is not equal to the logical row size in bytes. "
                    << "Texture alignment requirement is \""
                    << devProp.textureAlignment << "\". "
                    << std::endl;
            }

            if( memory._hostPtr != NULL || memory._devArray != NULL )
                memory._syncOp |= osgCompute::SYNC_DEVICE;

            return true;
        }


        return false;
    }

    //------------------------------------------------------------------------------
    bool Memory::sync( unsigned int mapping )
    {
        cudaError res;

        ////////////////////
        // RECEIVE HANDLE //
        ////////////////////
        MemoryObject* memoryPtr = dynamic_cast<MemoryObject*>( object() );
        if( !memoryPtr )
            return false;
        MemoryObject& memory = *memoryPtr;

        /////////////////
        // SYNC MEMORY //
        /////////////////
        /////////////////
        // SYNC MEMORY //
        /////////////////
        if( (mapping & osgCompute::MAP_DEVICE_ARRAY) == osgCompute::MAP_DEVICE_ARRAY )
        {
            if( !(memory._syncOp & osgCompute::SYNC_ARRAY) )
                return true;

            if( ((memory._syncOp & osgCompute::SYNC_DEVICE) && memory._hostPtr == NULL) ||
                ((memory._syncOp & osgCompute::SYNC_HOST) && memory._devPtr == NULL) ||
                ((memory._syncOp & osgCompute::SYNC_DEVICE) && (memory._syncOp & osgCompute::SYNC_HOST)) )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::sync()]: no current memory found."
                    << std::endl;

                return false;
            }

            if( (memory._syncOp & osgCompute::SYNC_DEVICE) )
            {
                // Copy from host memory
                if( getNumDimensions() < 2 )
                {
                    res = cudaMemcpyToArray( memory._devArray, 0, 0, memory._hostPtr, getByteSize(), cudaMemcpyHostToDevice);
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: cudaMemcpyToArray() failed."
                            << " " << cudaGetErrorString( res ) << "."
                            << std::endl;

                        return false;
                    }
                }
                if( getNumDimensions() == 2 ) 
                {
                    res = cudaMemcpy2DToArray( memory._devArray, 0, 0, memory._hostPtr, 
                        getDimension(0)*getElementSize(), getDimension(0)*getElementSize(), getDimension(1), 
                        cudaMemcpyHostToDevice );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: cudaMemcpy2DToArray() failed."
                            << " " << cudaGetErrorString( res ) << "."
                            << std::endl;

                        return false;
                    }
                }
                else
                {
                    cudaMemcpy3DParms memCpyParams = {0};
                    memCpyParams.dstArray = memory._devArray;
                    memCpyParams.kind = cudaMemcpyHostToDevice;
                    memCpyParams.srcPtr = make_cudaPitchedPtr(memory._hostPtr, getDimension(0)*getElementSize(), getDimension(0), getDimension(1));

                    cudaExtent arrayExtent = {0};
                    arrayExtent.width = getDimension(0);
                    arrayExtent.height = getDimension(1);
                    arrayExtent.depth = getDimension(2);

                    memCpyParams.extent = arrayExtent;

                    res = cudaMemcpy3D( &memCpyParams );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << "[osgCuda::Memory::sync()]: cudaMemcpy3D() failed."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
            }
            else
            {
                // Copy from device memory
                if( getNumDimensions() < 2 )
                {
                    res = cudaMemcpyToArray( memory._devArray, 0, 0, memory._devPtr, getByteSize(), cudaMemcpyDeviceToDevice);
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: cudaMemcpyToArray() failed."
                            << " " << cudaGetErrorString( res ) << "."
                            << std::endl;

                        return false;
                    }
                }
                else if( getNumDimensions() == 2 ) 
                {
                    res = cudaMemcpy2DToArray( memory._devArray, 0, 0, memory._devPtr, 
                                               memory._pitch,  getDimension(0)*getElementSize(), getDimension(1), 
                                               cudaMemcpyDeviceToDevice );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: cudaMemcpy2DToArray() failed."
                            << " " << cudaGetErrorString( res ) << "."
                            << std::endl;

                        return false;
                    }
                }
                else
                {
                    cudaMemcpy3DParms memCpyParams = {0};
                    memCpyParams.dstArray = memory._devArray;
                    memCpyParams.kind = cudaMemcpyDeviceToDevice;
                    memCpyParams.srcPtr = make_cudaPitchedPtr(memory._devPtr, memory._pitch, getDimension(0), getDimension(1));

                    cudaExtent arrayExtent = {0};
                    arrayExtent.width = getDimension(0);
                    arrayExtent.height = getDimension(1);
                    arrayExtent.depth = getDimension(2);

                    memCpyParams.extent = arrayExtent;

                    res = cudaMemcpy3D( &memCpyParams );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << "[osgCuda::Memory::sync()]: cudaMemcpy3D() failed."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
            }

            memory._syncOp = memory._syncOp ^ osgCompute::SYNC_ARRAY;
            return true;
        }
        else if( mapping & osgCompute::MAP_DEVICE )
        {
            if( !(memory._syncOp & osgCompute::SYNC_DEVICE) )
                return true;

            if( ((memory._syncOp & osgCompute::SYNC_ARRAY) && memory._hostPtr == NULL) ||
                ((memory._syncOp & osgCompute::SYNC_HOST) && memory._devArray == NULL) ||
                ((memory._syncOp & osgCompute::SYNC_ARRAY) && (memory._syncOp & osgCompute::SYNC_HOST)) )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::sync()]: no current memory found."
                    << std::endl;

                return false;
            }

            if( (memory._syncOp & osgCompute::SYNC_HOST) )
            {
                // Copy from array
                if( getNumDimensions() == 1 )
                {
                    res = cudaMemcpyFromArray( memory._devPtr, memory._devArray, 0, 0, getByteSize(), cudaMemcpyDeviceToDevice );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]:  error during cudaMemcpyFromArray() to host memory."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
                else if( getNumDimensions() == 2 )
                {
                    res = cudaMemcpy2DFromArray(
                        memory._devPtr,
                        memory._pitch,
                        memory._devArray,
                        0, 0,
                        getDimension(0)* getElementSize(),
                        getDimension(1),
                        cudaMemcpyDeviceToDevice );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: error during cudaMemcpy2DFromArray() to host memory."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
                else
                {
                    cudaPitchedPtr pitchPtr = {0};
                    pitchPtr.pitch = memory._pitch;
                    pitchPtr.ptr = memory._devPtr;
                    pitchPtr.xsize = getDimension(0);
                    pitchPtr.ysize = getDimension(1);

                    cudaExtent extent = {0};
                    extent.width = getDimension(0);
                    extent.height = getDimension(1);
                    extent.depth = getDimension(2);

                    cudaMemcpy3DParms copyParams = {0};
                    copyParams.srcArray = memory._devArray;
                    copyParams.dstPtr = pitchPtr;
                    copyParams.extent = extent;
                    copyParams.kind = cudaMemcpyDeviceToDevice;

                    res = cudaMemcpy3D( &copyParams );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: error during cudaMemcpy3D() to host memory."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
            }
            else
            {
                if( getNumDimensions() == 3 )
                {
                    cudaMemcpy3DParms memCpyParams = {0};
                    memCpyParams.dstPtr = make_cudaPitchedPtr(memory._devPtr,memory._pitch, getDimension(0), getDimension(1));
                    memCpyParams.kind = cudaMemcpyHostToDevice;
                    memCpyParams.srcPtr = make_cudaPitchedPtr(memory._hostPtr,getElementSize()*getDimension(0), getDimension(0), getDimension(1));

                    cudaExtent arrayExtent = {0};
                    arrayExtent.width = getElementSize()*getDimension(0);
                    arrayExtent.height = getDimension(1);
                    arrayExtent.depth = getDimension(2);

                    memCpyParams.extent = arrayExtent;

                    res = cudaMemcpy3D( &memCpyParams );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << "[osgCuda::Memory::sync()]: cudaMemcpy3D() to device failed."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
                else if( getNumDimensions() == 2 )
                {
                    res = cudaMemcpy2D( memory._devPtr, memory._pitch, memory._hostPtr, getElementSize()*getDimension(0), 
                        getDimension(0)*getElementSize(), getDimension(1), cudaMemcpyHostToDevice );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: cudaMemcpy2D() to device failed."
                            << " " << cudaGetErrorString( res ) << "."
                            << std::endl;

                        return false;
                    }
                }
                else
                {
                    res = cudaMemcpy( memory._devPtr, memory._hostPtr, getByteSize(), cudaMemcpyHostToDevice );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()] \""<<getName()<<"\": cudaMemcpy() to device failed."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;
                        return false;
                    }
                }
            }

            memory._syncOp = memory._syncOp ^ osgCompute::SYNC_DEVICE;
            return true;
        }
        else if( mapping & osgCompute::MAP_HOST )
        {
            if( !(memory._syncOp & osgCompute::SYNC_HOST) )
                return true;

            if( ((memory._syncOp & osgCompute::SYNC_ARRAY) && memory._devPtr == NULL) ||
                ((memory._syncOp & osgCompute::SYNC_DEVICE) && memory._devArray == NULL) ||
                ((memory._syncOp & osgCompute::SYNC_ARRAY) && (memory._syncOp & osgCompute::SYNC_DEVICE)) )
            {
                osg::notify(osg::FATAL)
                    << getName() << " [osgCuda::Memory::sync()]: no current memory found."
                    << std::endl;

                return false;
            }

            if( (memory._syncOp & osgCompute::SYNC_DEVICE) )
            {
                // Copy from array
                if( getNumDimensions() == 1 )
                {
                    res = cudaMemcpyFromArray( memory._hostPtr, memory._devArray, 0, 0, getByteSize(), cudaMemcpyDeviceToHost );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]:  error during cudaMemcpyFromArray() to host memory."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
                else if( getNumDimensions() == 2 )
                {
                    res = cudaMemcpy2DFromArray(
                        memory._hostPtr,
                        getDimension(0) * getElementSize(),
                        memory._devArray,
                        0, 0,
                        getDimension(0)*getElementSize(),
                        getDimension(1),
                        cudaMemcpyDeviceToHost );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: error during cudaMemcpy2DFromArray() to host memory."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
                else
                {
                    cudaPitchedPtr pitchPtr = {0};
                    pitchPtr.pitch = getDimension(0)*getElementSize();
                    pitchPtr.ptr = memory._hostPtr;
                    pitchPtr.xsize = getDimension(0);
                    pitchPtr.ysize = getDimension(1);

                    cudaExtent extent = {0};
                    extent.width = getDimension(0);
                    extent.height = getDimension(1);
                    extent.depth = getDimension(2);

                    cudaMemcpy3DParms copyParams = {0};
                    copyParams.srcArray = memory._devArray;
                    copyParams.dstPtr = pitchPtr;
                    copyParams.extent = extent;
                    copyParams.kind = cudaMemcpyDeviceToHost;

                    res = cudaMemcpy3D( &copyParams );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: error during cudaMemcpy3D() to host memory."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;

                    }
                }
            }
            else
            {
                if( getNumDimensions() == 3 )
                {
                    cudaMemcpy3DParms memCpyParams = {0};
                    memCpyParams.dstPtr = make_cudaPitchedPtr(memory._hostPtr,getElementSize()*getDimension(0), getDimension(0), getDimension(1));
                    memCpyParams.kind = cudaMemcpyDeviceToHost;
                    memCpyParams.srcPtr = make_cudaPitchedPtr(memory._devPtr,memory._pitch, getDimension(0), getDimension(1));

                    cudaExtent arrayExtent = {0};
                    arrayExtent.width = getElementSize()*getDimension(0);
                    arrayExtent.height = getDimension(1);
                    arrayExtent.depth = getDimension(2);

                    memCpyParams.extent = arrayExtent;

                    res = cudaMemcpy3D( &memCpyParams );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << "[osgCuda::Memory::sync()]: cudaMemcpy3D() to host failed."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;

                        return false;
                    }
                }
                else if( getNumDimensions() == 2 )
                {
                    res = cudaMemcpy2D( memory._hostPtr, getElementSize()*getDimension(0), memory._devPtr, memory._pitch, 
                        getDimension(0)*getElementSize(), getDimension(1), cudaMemcpyDeviceToHost );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()]: cudaMemcpy2D() to host failed."
                            << " " << cudaGetErrorString( res ) << "."
                            << std::endl;

                        return false;
                    }
                }
                else
                {
                    res = cudaMemcpy( memory._hostPtr, memory._devPtr, getByteSize(), cudaMemcpyDeviceToHost );
                    if( cudaSuccess != res )
                    {
                        osg::notify(osg::FATAL)
                            << getName() << " [osgCuda::Memory::sync()] \""<<getName()<<"\": cudaMemcpy() to host failed."
                            << " " << cudaGetErrorString( res ) <<"."
                            << std::endl;
                        return false;
                    }
                }
            }

            memory._syncOp = memory._syncOp ^ osgCompute::SYNC_HOST;
            return true;
        }

        return false;
    }

    //------------------------------------------------------------------------------
    void Memory::setImage( osg::Image* image )
    {
        if( !osgCompute::Resource::isClear() && image != NULL )
        {
            if( image->getNumMipmapLevels() > 1 )
            {
                osg::notify(osg::WARN)
                    << getName() << " [osgCuda::Memory::setImage()] \""<<getName()<<"\": image \""
                    << image->getName() << "\" uses MipMaps which are currently"
                    << "not supported." 
                    << std::endl;

                return;
            }

            if( image->getTotalSizeInBytes() != getByteSize() )
            {
                osg::notify(osg::WARN)
                    << getName() << " [osgCuda::Memory::setImage()] \""<<getName()<<"\": size of image \""
                    << image->getName() << "\" is wrong."
                    << std::endl;

                return;
            }
        }


        _image = image;
        resetModifiedCounts();
    }

    //------------------------------------------------------------------------------
    osg::Image* Memory::getImage()
    {
        return _image.get();
    }

    //------------------------------------------------------------------------------
    const osg::Image* Memory::getImage() const
    {
        return _image.get();
    }

    //------------------------------------------------------------------------------
    cudaChannelFormatDesc& Memory::getChannelFormatDesc()
    {
        return _formatDesc;
    }

    //------------------------------------------------------------------------------
    const cudaChannelFormatDesc& Memory::getChannelFormatDesc() const
    {
        return _formatDesc;
    }

    //------------------------------------------------------------------------------
    void Memory::setChannelFormatDesc(cudaChannelFormatDesc& formatDesc)
    {
        if( !osgCompute::Resource::isClear() )
            return;

        _formatDesc = formatDesc;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////
    // PROTECTED FUNCTIONS //////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////
    //------------------------------------------------------------------------------
    void Memory::clearLocal()
    {
        _image = NULL;
        memset( &_formatDesc, 0x0, sizeof(cudaChannelFormatDesc) );
    }

    //------------------------------------------------------------------------------
    unsigned int Memory::computePitch() const
    {
        // Proof paramters
        if( getDimension(0) == 0 || getElementSize() == 0 ) 
            return 0;

        int device;
        cudaGetDevice( &device );
        cudaDeviceProp devProp;
        cudaGetDeviceProperties( &devProp, device );

        unsigned int remainingAlignmentBytes = (getDimension(0)*getElementSize()) % devProp.textureAlignment;
        if( remainingAlignmentBytes != 0 )
            return (getDimension(0)*getElementSize()) + (devProp.textureAlignment-remainingAlignmentBytes);
        else
            return (getDimension(0)*getElementSize()); // no additional bytes required.
    }

    //------------------------------------------------------------------------------
    osgCompute::MemoryObject* Memory::createObject() const
    {
        return new MemoryObject;
    }

    //------------------------------------------------------------------------------
    void Memory::resetModifiedCounts() const
    {
        ////////////////////
        // RECEIVE HANDLE //
        ////////////////////
        const MemoryObject* memoryPtr = dynamic_cast<const MemoryObject*>( object() );
        if( !memoryPtr )
            return;
        MemoryObject& memory = const_cast<MemoryObject&>(*memoryPtr);

        ///////////////////
        // RESET COUNTER //
        ///////////////////
        memory._modifyCount = UINT_MAX;
    }
}
