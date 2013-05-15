#ifndef Importer_MTXImporter_H
#define Importer_MTXImporter_H
//-----------------------------------------------------------------------------
#include "Importer/StreamImporter.h"
//-----------------------------------------------------------------------------

namespace Importer 
{

	/**
	*  \class MTXImporter
	*  \brief Imports graph data from data in MTX format.
	*  \author Vladimir Polak
	*  \date 20. 4. 2013
	*/
	class MTXImporter: public StreamImporter 
	{
	public:

		/**
		*  \fn public virtual destructor ~MTXImporter
		*  \brief Destroys the MTXImporter object
		*/
		virtual ~MTXImporter (void) {};
		
		/**
         * \fn public virtual import
         * \brief Reads mtx file and creates graph
		 * \param[out] context Reference to the context
		 * \return bool Returns true if graph successfully created, false otherwise.
         */
		virtual bool import ( ImporterContext &context );

	};

}

#endif // Importer_MTXImporter_H
