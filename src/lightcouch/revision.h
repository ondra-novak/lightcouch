/*
 * revision.h
 *
 *  Created on: May 28, 2016
 *      Author: ondra
 */

#ifndef LIBS_LIGHTCOUCH_SRC_LIGHTCOUCH_REVISION_H_
#define LIBS_LIGHTCOUCH_SRC_LIGHTCOUCH_REVISION_H_
#include "lightspeed/base/memory/staticAlloc.h"
#include "lightspeed/base/compare.h"
#include "lightspeed/base/containers/autoArray.h"
#include "lightspeed/base/containers/string.h"
namespace LightCouch {

using namespace LightSpeed;

class Revision: public Comparable<Revision> {
public:
	Revision();
	Revision(natural revId, ConstStrA tag);
	Revision(ConstStrA revStr);

	natural getRevId() const {return revId;}
	ConstStrA getTag() const {return tag;}

	static natural getRevId(ConstStrA rev);
	static ConstStrA getTag(ConstStrA rev);


	StringA toString() const;


	CompareResult compare(const Revision &other) const;

protected:
	natural revId;
	AutoArray<char, StaticAlloc<32> > tag;

};

}

#endif /* LIBS_LIGHTCOUCH_SRC_LIGHTCOUCH_REVISION_H_ */
