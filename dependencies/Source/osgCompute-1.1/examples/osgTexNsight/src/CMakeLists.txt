#########################################################################
# Set target name und set path to data folder of the target
#########################################################################

SET(TARGETNAME osgTexNsight)
SET(ORIGTARGETNAME osgTexDemo)
SET(TARGET_DATA_PATH "${DATA_PATH}/${ORIGTARGETNAME}")

SET(HEADER_PATH ${osgCompute_SOURCE_DIR}/examples/${ORIGTARGETNAME}/include)
SET(SOURCE_PATH ${osgCompute_SOURCE_DIR}/examples/${ORIGTARGETNAME}/src)

#SET(ADDITIONAL_HEADER_PATH ${osgCompute_SOURCE_DIR}/examples/${TARGETNAME}/include)
#SET(ADDITIONAL_SOURCE_PATH ${osgCompute_SOURCE_DIR}/examples/${TARGETNAME}/src)


#in Cuda Nsight probably not needed anymore (-> only needed on CPU, with Emulation Mode)
# Remove /STACK:10000000 set by CMake. This value for stack size
# is very high, limiting the number of threads we can spawn.
# Default value used by Windows is 1MB which is good enough.
#
#STRING(REGEX REPLACE "/STACK:[0-9]+" ""
#    CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")
    
    
#########################################################################
# Do necessary checking stuff (check for other libraries to link against ...)
#########################################################################

# find osg
INCLUDE(Findosg)
INCLUDE(FindosgUtil)
INCLUDE(FindOpenThreads)
INCLUDE(FindosgViewer)
INCLUDE(FindosgGA)
INCLUDE(FindosgDB)
# check for cuda
INCLUDE(FindCudaNsight)

#configure cuda debugging with nsight - see also main cmake file
# use this for additional application specific options
SET(CUDA_NSIGHT_OPTIONS ${CUDA_NSIGHT_OPTIONS} 
   # add your options here
)
ADD_DEFINITIONS(-DUSE_CUDA_DEBUGGING)



#########################################################################
# Set basic include directories
#########################################################################

# set include dirs
INCLUDE_DIRECTORIES(
    ${HEADER_PATH}
    ${OSG_INCLUDE_DIR}
    ${CUDA_NSIGHT_INCLUDE_PATH}
)


#########################################################################
# Collect header and source files and process macros
#########################################################################

# collect all headers

SET(TARGET_H
	 ${HEADER_PATH}/TexStreamer
)


SET(MY_CUDA_SOURCE_FILES
	${SOURCE_PATH}/TexStreamer.cu
)

# Use the WRAP_CUDA macro.
WRAP_CUDA_NSIGHT( CUDA_FILES ${MY_CUDA_SOURCE_FILES} )

# collect the sources
SET(TARGET_SRC
	${SOURCE_PATH}/main.cpp
	${SOURCE_PATH}/TexStreamer.cpp
	${CUDA_FILES}
    ${MY_CUDA_SOURCE_FILES} 
)

#########################################################################
# Setup groups for resources (mainly for MSVC project folders)
#########################################################################

# Setup groups for headers (especially for files with no extension)
SOURCE_GROUP(
    "Header Files"
    FILES ${TARGET_H}     
)

# Setup groups for sources 
SOURCE_GROUP(
    "Source Files"
    FILES ${TARGET_SRC}
)

# Setup groups for resources 

# First: collect the necessary files which were not collected up to now
# Therefore, fill the following variables: 
# MY_ICE_FILES - MY_MODEL_FILES - MY_SHADER_FILES - MY_UI_FILES - MY_XML_FILES

# collect shader files
SET(MY_SHADER_FILES
)

# finally, use module to build groups
INCLUDE(GroupInstall)


# now set up the ADDITIONAL_FILES variable to ensure that the files will be visible in the project
SET(ADDITIONAL_FILES
)



#########################################################################
# Setup libraries to link against
#########################################################################

# put here own project libraries, for example. (Attention: you do not have
# to differentiate between debug and optimized: this is done automatically by cmake
SET(TARGET_ADDITIONAL_LIBRARIES
	osgCompute
	osgCudaNsight
)


# put here the libraries which are collected in a variable (i.e. most of the FindXXX scrips)
# the macro (LINK_WITH_VARIABLES) ensures that also the ${varname}_DEBUG names will resolved correctly
SET(TARGET_VARS_LIBRARIES 	
	OPENTHREADS_LIBRARY
	OSG_LIBRARY
	OSGUTIL_LIBRARY
	OSGVIEWER_LIBRARY
	OSGGA_LIBRARY
	OSGDB_LIBRARY
    CUDA_NSIGHT_RUNTIME_LIBRARY
)
	

#########################################################################
# Example setup and install
#########################################################################

# this is a user definded macro which does all the work for us
# it also takes into account the variables TARGET_SRC,
# TARGET_H and TARGET_ADDITIONAL_LIBRARIES and TARGET_VARS_LIBRARIES and ADDITIONAL_FILES
SETUP_EXAMPLE_WITH_OPENGL_LINKING(${TARGETNAME})
