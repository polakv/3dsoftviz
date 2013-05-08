#ifndef Importer_MTXImporter_H
#define Importer_MTXImporter_H
//-----------------------------------------------------------------------------
#include "Importer/StreamImporter.h"
//-----------------------------------------------------------------------------

namespace Importer {

/**
 * \brief Imports graph data from data in Matrix Market format.
 */
class MTXImporter
	: public StreamImporter {

public:

	/***/
	virtual ~MTXImporter (void) {};
	
	/**
	*\brief Read rsf file and create graph
	*/
	virtual bool import (
		ImporterContext &context
	);

}; // class

} // namespace

#endif // Importer_MTXImporter_H
