/* osgCompute - Copyright (C) 2008-2009 SVT Group
*                                                                     
* This library is free software; you can redistribute it and/or modify
* it under the terms of the GNU Lesser General Public License as
* published by the Free Software Foundation; either version 3 of
* the License, or (at your option) any later version.
*                                                                     
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of 
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Lesse General Public License for more details.
*
* The full license is in LICENSE file included with this distribution.
*/
#include <iostream>
#include <sstream>
#include <osg/ArgumentParser>
#include <osg/Texture2D>
#include <osg/Viewport>
#include <osg/AlphaFunc>
#include <osg/PolygonMode>
#include <osg/Geometry>
#include <osg/Point>
#include <osg/Array>
#include <osg/PointSprite>
#include <osg/Geometry>
#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osgDB/FileUtils>
#include <osgDB/Registry>
#include <osgViewer/Viewer>
#include <osgViewer/ViewerEventHandlers>
#include <osgCuda/Computation>
#include <osgCuda/Memory>
#include <osgCuda/Geometry>

//------------------------------------------------------------------------------
osg::Geode* getBoundingBox( osg::Vec3& bbmin, osg::Vec3& bbmax )
{
    osg::Geometry* bbgeom = new osg::Geometry;

    // vertices
    osg::Vec3Array* vertices = new osg::Vec3Array();

    osg::Vec3 center = (bbmin + bbmax) * 0.5f;
    osg::Vec3 radiusX( bbmax.x() - center.x(), 0, 0 );
    osg::Vec3 radiusY( 0, bbmax.y() - center.y(), 0 );
    osg::Vec3 radiusZ( 0, 0, bbmax.z() - center.z() );

    vertices->push_back( center - radiusX - radiusY - radiusZ ); // 0
    vertices->push_back( center + radiusX - radiusY - radiusZ ); // 1
    vertices->push_back( center + radiusX + radiusY - radiusZ ); // 2
    vertices->push_back( center - radiusX + radiusY - radiusZ ); // 3
    vertices->push_back( center - radiusX - radiusY + radiusZ ); // 4
    vertices->push_back( center + radiusX - radiusY + radiusZ ); // 5
    vertices->push_back( center + radiusX + radiusY + radiusZ ); // 6
    vertices->push_back( center - radiusX + radiusY + radiusZ ); // 7
    bbgeom->setVertexArray( vertices );

    // indices
    osg::DrawElementsUShort* indices = new osg::DrawElementsUShort(GL_LINES);

    indices->push_back(0);
    indices->push_back(1);
    indices->push_back(1);
    indices->push_back(2);
    indices->push_back(2);
    indices->push_back(3);
    indices->push_back(3);
    indices->push_back(0);

    indices->push_back(4);
    indices->push_back(5);
    indices->push_back(5);
    indices->push_back(6);
    indices->push_back(6);
    indices->push_back(7);
    indices->push_back(7);
    indices->push_back(4);

    indices->push_back(1);
    indices->push_back(5);
    indices->push_back(2);
    indices->push_back(6);
    indices->push_back(3);
    indices->push_back(7);
    indices->push_back(0);
    indices->push_back(4);
    bbgeom->addPrimitiveSet( indices );

    // color
    osg::Vec4Array* color = new osg::Vec4Array;
    color->push_back( osg::Vec4(0.5f, 0.5f, 0.5f, 1.f) );
    bbgeom->setColorArray( color );
    bbgeom->setColorBinding( osg::Geometry::BIND_OVERALL );

    ////////////////
    // SETUP BBOX //
    ////////////////
    osg::Geode* bbox = new osg::Geode;
    bbox->addDrawable( bbgeom );

    // Disable lighting
    bbox->getOrCreateStateSet()->setMode( GL_LIGHTING, osg::StateAttribute::OFF );

    return bbox;
}

//------------------------------------------------------------------------------
osg::Geode* getGeode( unsigned int numParticles )
{
    osg::Geode* geode = new osg::Geode;

    //////////////
    // GEOMETRY //
    //////////////
    osg::ref_ptr<osgCuda::Geometry> ptclGeom = new osgCuda::Geometry;

    // Initialize the Particles
    osg::Vec4Array* coords = new osg::Vec4Array(numParticles);
    for( unsigned int v=0; v<coords->size(); ++v )
        (*coords)[v].set(-1,-1,-1,0);

    ptclGeom->setVertexArray(coords);
    ptclGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS,0,coords->size()));
    ptclGeom->addIdentifier( "PTCL_BUFFER" );

    // Add particles
    geode->addDrawable( ptclGeom.get() );

    ////////////
    // SPRITE //
    ////////////
    // increase point size within shader
    geode->getOrCreateStateSet()->setMode(GL_VERTEX_PROGRAM_POINT_SIZE, osg::StateAttribute::ON);

    osg::PointSprite* sprite = new osg::PointSprite();
    geode->getOrCreateStateSet()->setTextureAttributeAndModes(0, sprite, osg::StateAttribute::ON);

    geode->getOrCreateStateSet()->setAttribute( new osg::AlphaFunc( osg::AlphaFunc::GREATER, 0.1f) );
    geode->getOrCreateStateSet()->setMode( GL_ALPHA_TEST, GL_TRUE );

    ////////////
    // SHADER //
    ////////////
    osg::Program* program = new osg::Program;

    const std::string vtxShader=
        "uniform vec2 pixelsize;                                                                \n"
        "                                                                                       \n"
        "void main(void)                                                                        \n"
        "{                                                                                      \n"
        "   vec4 worldPos = vec4(gl_Vertex.x,gl_Vertex.y,gl_Vertex.z,1.0);                      \n"
        "   vec4 projPos = gl_ModelViewProjectionMatrix * worldPos;                             \n"
        "                                                                                       \n"
        "   float dist = projPos.z / projPos.w;                                                 \n"
        "   float distAlpha = (dist+1.0)/2.0;                                                   \n"
        "   gl_PointSize = pixelsize.y - distAlpha * (pixelsize.y - pixelsize.x);               \n"
        "                                                                                       \n"
        "   gl_Position = projPos;                                                              \n"
        "}                                                                                      \n";
    program->addShader( new osg::Shader(osg::Shader::VERTEX, vtxShader ) );

    const std::string frgShader=
        "void main (void)                                                                       \n"
        "{                                                                                      \n"
        "   vec4 result;                                                                        \n"
        "                                                                                       \n"
        "   vec2 tex_coord = gl_TexCoord[0].xy;                                                 \n"
        "   tex_coord.y = 1.0-tex_coord.y;                                                      \n"
        "   float d = 2.0*distance(tex_coord.xy, vec2(0.5, 0.5));                               \n"
        "   result.a = step(d, 1.0);                                                            \n"
        "                                                                                       \n"
        "   vec3 eye_vector = normalize(vec3(0.0, 0.0, 1.0));                                   \n"
        "   vec3 light_vector = normalize(vec3(2.0, 2.0, 1.0));                                 \n"
        "   vec3 surface_normal = normalize(vec3(2.0*                                           \n"
        "           (tex_coord.xy-vec2(0.5, 0.5)), sqrt(1.0-d)));                               \n"
        "   vec3 half_vector = normalize(eye_vector+light_vector);                              \n"
        "                                                                                       \n"
        "   float specular = dot(surface_normal, half_vector);                                  \n"
        "   float diffuse  = dot(surface_normal, light_vector);                                 \n"
        "                                                                                       \n"
        "   vec4 lighting = vec4(0.75, max(diffuse, 0.0), pow(max(specular, 0.0), 40.0), 0.0);  \n"
        "                                                                                       \n"
        "   result.rgb = lighting.x*vec3(0.2, 0.8, 0.2)+lighting.y*vec3(0.6, 0.6, 0.6)+         \n"
        "   lighting.z*vec3(0.25, 0.25, 0.25);                                                  \n"
        "                                                                                       \n"
        "   gl_FragColor = result;                                                              \n"
        "}                                                                                      \n";

    program->addShader( new osg::Shader( osg::Shader::FRAGMENT, frgShader ) );


    geode->getOrCreateStateSet()->setAttribute(program);

    // Screen resolution for particle sprite
    osg::Uniform* pixelsize = new osg::Uniform();
    pixelsize->setName( "pixelsize" );
    pixelsize->setType( osg::Uniform::FLOAT_VEC2 );
    pixelsize->set( osg::Vec2(1.0f,50.0f) );
    geode->getOrCreateStateSet()->addUniform( pixelsize );
    geode->setCullingActive( false );

    return geode;
}

//------------------------------------------------------------------------------
osg::ref_ptr<osgCompute::Computation> setupComputation()
{
    osg::ref_ptr<osgCompute::Computation> computationEmitter = new osgCuda::Computation;
    computationEmitter->setName( "emit particles computation" );
    osg::ref_ptr<osgCompute::Computation> computationTracer = new osgCuda::Computation;
    computationTracer->setName( "trace particles computation" );

    // Execute the computation during the update traversal before the subgraph is handled. 
    // This is the default behaviour. Use the following line to execute the computation in
    // the rendering traversal after the subgraph has been rendered.
    //osgCompute::Computation::ComputeOrder order = osgCompute::Computation::PRERENDER_AFTERCHILDREN;
    osgCompute::Computation::ComputeOrder order = osgCompute::Computation::UPDATE_BEFORECHILDREN;
    computationEmitter->setComputeOrder( order );
    computationTracer->setComputeOrder( order );

    ////////////////////
    // SETUP HIERACHY //
    ////////////////////
    osgCompute::Module* ptclTracer = osgCompute::Module::loadModule("osgcuda_ptcltracer");
    if( ptclTracer )
    {
        ptclTracer->addIdentifier( "osgcuda_ptcltracer" );
        computationTracer->addModule( *ptclTracer );
    }

    osgCompute::Module* ptclEmitter = osgCompute::Module::loadModule("osgcuda_ptclemitter");
    if( ptclEmitter )
    {
        ptclEmitter->addIdentifier( "osgcuda_ptclemitter" );
        computationEmitter->addModule( *ptclEmitter );
    }
    computationTracer->addChild( computationEmitter );

    // Write this computation to file
    // osgDB::writeNodeFile( *computationTracer, "tracedemo.osgt" );

    return computationTracer;
}

//------------------------------------------------------------------------------
osg::ref_ptr<osgCompute::Computation> loadComputation()
{
    osg::ref_ptr<osgCompute::Computation> computationNode;

    std::string dataFile = osgDB::findDataFile( "osgTraceDemo/scenes/tracedemo.osgt" );
    if( !dataFile.empty() )
        computationNode = dynamic_cast<osgCuda::Computation*>( osgDB::readNodeFile( dataFile ) );

    return computationNode;
}

//------------------------------------------------------------------------------
int main(int argc, char *argv[])
{
    osg::setNotifyLevel( osg::WARN );
    osg::ArgumentParser arguments(&argc, argv);
    osgViewer::Viewer viewer(arguments);

    osg::Vec3 bbmin(-1.f,-1.f,-1.f);
    osg::Vec3 bbmax(1.f,1.f,1.f);
    unsigned int numParticles = 64000;

    /////////////////
    // COMPUTATION //
    /////////////////
    // Please note that loading need the modules to be
    // INSTALLED first. Otherwise the serializer cannot
    // find the respective modules.
    osg::ref_ptr<osgCompute::Computation> computation = loadComputation();
    if( !computation.valid() ) computation = setupComputation();


    /////////////////
    // SETUP SCENE //
    /////////////////
    osg::Group* scene = new osg::Group;
    scene->addChild( computation );
    scene->addChild( getBoundingBox( bbmin, bbmax ) );
    scene->addChild( getGeode( numParticles ) );


    //////////////////////
    // RESOURCE VISITOR //
    //////////////////////
    // The resource-visitor will distribute all resources over
    // the scene.
    osg::ref_ptr<osgCompute::ResourceVisitor> rv = new osgCompute::ResourceVisitor;
    rv->apply( *scene );

    //////////////////
    // SETUP VIEWER //
    //////////////////
    // You must use the single threaded version since osgCompute currently
    // does only support single threaded applications. Please ask in the
    // forum for the multi-threaded version if you need it.
    viewer.setThreadingModel(osgViewer::Viewer::SingleThreaded);
    viewer.setReleaseContextAtEndOfFrameHint(false);
    viewer.getCamera()->setComputeNearFarMode( osg::Camera::DO_NOT_COMPUTE_NEAR_FAR );
    viewer.getCamera()->setClearColor( osg::Vec4(0.15, 0.15, 0.15, 1.0) );
    viewer.setUpViewInWindow( 50, 50, 640, 480);
    viewer.setSceneData( scene );
    viewer.addEventHandler(new osgViewer::StatsHandler);

    return viewer.run();
}
