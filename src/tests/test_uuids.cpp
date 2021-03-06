/*
 * test_uuids.cpp
 *
 *  Created on: 10. 3. 2016
 *      Author: ondra
 */

#include "lightspeed/base/framework/testapp.h"
#include "lightspeed/base/text/textstream.tcc"
#include "lightspeed/base/streams/fileiobuff.tcc"
#include "../lightcouch/couchDB.h"

#include "test_common.h"

#include "lightspeed/base/containers/set.tcc"

#include "lightspeed/mt/thread.h"
namespace LightCouch {
using namespace LightSpeed;
using namespace BredyHttpClient;



static void genFastUUIDS(PrintTextA &print) {

	Set<StringA> uuidmap;
	CouchDB db(getTestCouch());
	ConsoleA out;

	for (natural i = 0; i < 50; i++) {
		StringA uuid = db.genUID("test-");
		out.print("%1\n") << uuid;
		uuidmap.insert(uuid);
		Thread::sleep(100);
	}
	print("%1") << uuidmap.size();
}



defineTest test_genfastuuids("couchdb.genfastuid","50",&genFastUUIDS);
}
