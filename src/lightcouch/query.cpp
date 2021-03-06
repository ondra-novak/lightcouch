/*
 * query.cpp
 *
 *  Created on: 12. 3. 2016
 *      Author: ondra
 */

#include <lightspeed/base/text/textOut.tcc>
#include "query.h"
#include "lightspeed/base/containers/autoArray.tcc"
#include "lightspeed/base/containers/map.tcc"

#include "collation.h"
#include "couchDB.h"
#include "query.tcc"

namespace LightCouch {

QueryBase::QueryBase(const Json &json, natural viewFlags)
:json(json),mode(mdKeys),viewFlags(viewFlags) {
	reset();
}


QueryBase& QueryBase::reset() {
	curKeySet.clear();
	startkey = endkey = keys = null;
	mode = mdKeys;
	staleMode = viewFlags & View::stale?smStale:(
			viewFlags & View::updateAfter?smUpdateAfter:smUpdate);

	groupLevel = (viewFlags & View::reduce)?((viewFlags & View::groupLevelMask) / View::groupLevel):naturalNull  ;

	offset = 0;
	maxlimit = naturalNull;
	descent = (viewFlags & View::reverseOrder) != 0;
	forceArray = false;
	args = null;

	return *this;
}


QueryBase& QueryBase::selectKey(ConstValue key) {
	if (keys== nil) {
		keys = json.array();
	}
	keys.add(key);
	return *this;

}

QueryBase& QueryBase::fromKey(ConstValue key) {
	keys = nil;
	startkey = key;
	return *this;

}

QueryBase& QueryBase::toKey(ConstValue key) {
	keys = nil;
	endkey = key;
	return *this;
}

QueryBase& QueryBase::operator ()(ConstStrA key) {
	curKeySet.add(json(key));
	return *this;
}

QueryBase& QueryBase::operator ()(natural key) {
	curKeySet.add(json(key));
	return *this;
}

QueryBase& QueryBase::operator ()(integer key) {
	curKeySet.add(json(key));
	return *this;
}

QueryBase& QueryBase::operator ()(int key) {
	curKeySet.add(json(integer(key)));
	return *this;
}

QueryBase& QueryBase::operator ()(double key) {
	curKeySet.add(json(key));
	return *this;
}

QueryBase& QueryBase::operator ()(ConstValue key) {
	curKeySet.add(key);
	return *this;
}

QueryBase& QueryBase::operator ()(bool key) {
	curKeySet.add(json(key));
	return *this;
}

QueryBase& QueryBase::operator ()(const char *key) {
	curKeySet.add(json(ConstStrA(key)));
	return *this;
}



QueryBase& QueryBase::operator()(MetaValue metaValue) {
	switch (metaValue) {
	case any: {
		forceArray=true;
		JSON::Container s,e;

		switch(mode) {
				case mdKeys:
					startkey = endkey = keys = null;
					if (curKeySet.empty()) return *this;
					e = buildRangeKey(curKeySet);
					s = buildRangeKey(curKeySet);
					s.add(json(null));
					e.add(json("\xEF\xBF\xBF",null));
					startkey = s;
					endkey = e;
					curKeySet.clear();
					return *this;
				case mdStart:
					curKeySet.add(json(null));
					return *this;
				case mdEnd:
					curKeySet.add(json("\xEF\xBF\xBF\xEF\xBF\xBF\xEF\xBF\xBF\xEF\xBF\xBF",null));
					return *this;
			}
		}
	case wildcard: {
			switch (mode) {
			case mdKeys:
				startkey = endkey = keys = null;
				if (curKeySet.empty()) return *this;
				startkey = buildKey(curKeySet);
				curKeySet(curKeySet.length()-1) = json(
						StringA(curKeySet(curKeySet.length()-1)->getStringUtf8()
							+ConstStrA("\xEF\xBF\xBF\xEF\xBF\xBF\xEF\xBF\xBF\xEF\xBF\xBF")));
				endkey = buildKey(curKeySet);
				curKeySet.clear();
				return *this;
			case mdStart:
				return *this;
			case mdEnd:
				if (curKeySet.empty()) return *this;
				curKeySet(curKeySet.length()-1) = json(
						StringA(curKeySet(curKeySet.length()-1)->getStringUtf8()
							+ConstStrA("\xEF\xBF\xBF\xEF\xBF\xBF\xEF\xBF\xBF\xEF\xBF\xBF")));
				return *this;
			}

		}
	case isArray:
		forceArray = true;
		return *this;
	}
}

void QueryBase::appendCustomArg(UrlFormatter &fmt, ConstStrA key, ConstStrA value ) {
	fmt("&%1=%2") << key << value;
}


void QueryBase::appendCustomArg(UrlFormatter &fmt, ConstStrA key, const JSON::INode * value ) const {
	if (value->getType() == JSON::ndString) {
		appendCustomArg(fmt,key,CouchDB::urlencode(value->getStringUtf8()));
	} else {
		ConstStrA str = json.factory->toString(*value);
		appendCustomArg(fmt,key,CouchDB::urlencode(str));
	}
}

Result Query::exec() const {


	finishCurrent();

	StringA hlp;

	urlline.clear();
	urlline.blockWrite(viewDefinition.viewPath,true);
	TextOut<AutoArrayStream<char> &, SmallAlloc<256> > urlformat(urlline);
	if (groupLevel==naturalNull)  urlformat("?reduce=false");
	else if (keys == null || keys->length() == 1){
		urlformat("?group_level=%1") << groupLevel;
	} else if (groupLevel > 0){
		urlformat("?group=true") ;
	}

	if (descent) {
		urlformat("&descending=true");
	}

	if (viewDefinition.flags & View::includeDocs) {
		urlformat("&include_docs=true");
	}

	if (offset) {
		urlformat("&skip=%1") << offset;
	}

	if (maxlimit!=naturalNull) {
		urlformat("&limit=%1") << maxlimit;
	}

	if (!offset_doc.empty()) {
		urlformat("&startkey_docid=%1") << (hlp=CouchDB::urlencode(offset_doc));
	}

	if (viewDefinition.flags & View::updateSeq) {
		urlformat("&update_seq=true");
	}

	if (viewDefinition.flags & View::exludeEnd) {
		urlformat("&inclusive_end=false");
	}

	if (args == null) {
		for (natural i = 0, cnt = viewDefinition.args.length(); i < cnt; i++) {
			appendCustomArg(urlformat,viewDefinition.args[i].key,viewDefinition.args[i].value);
		}
	} else {
		for (natural i = 0, cnt = viewDefinition.args.length(); i < cnt; i++) {
			JSON::INode *nd = args->getPtr(viewDefinition.args[i].key);
			if (nd==0)
				appendCustomArg(urlformat,viewDefinition.args[i].key,viewDefinition.args[i].value);
		}
		for (JSON::Iterator iter = args->getFwIter(); iter.hasItems();) {
			const JSON::KeyValue &kv = iter.getNext();
			appendCustomArg(urlformat,kv->getStringUtf8(), kv);
		}
	}

	switch (staleMode) {
	case smUpdate:break;
	case smUpdateAfter: urlformat("&stale=update_after");break;
	case smStale: urlformat("&stale=ok");break;
	}

	ConstValue result;

	if (keys == nil) {

		if (startkey != nil) {
			urlformat(descent?"&endkey=%1":"&startkey=%1") << (hlp=CouchDB::urlencode(json.factory->toString(*startkey)));
		}
		if (endkey != nil) {
			urlformat(descent?"&startkey=%1":"&endkey=%1") << (hlp=CouchDB::urlencode(json.factory->toString(*endkey)));
		}

		result = db.requestGET(urlline.getArray());
	} else if (keys->length() == 1) {
		urlformat("&key=%1") << (hlp=CouchDB::urlencode(json.factory->toString(*(keys[0]))));
		result = db.requestGET(urlline.getArray());
	} else {
		if (viewDefinition.flags & View::forceGETMethod) {
			urlformat("&keys=%1") << (hlp=CouchDB::urlencode(json.factory->toString(*keys)));
			result = db.requestGET(urlline.getArray());
		} else {
			JSON::Container req = json("keys",keys);
			result = db.requestPOST(urlline.getArray(), req);
		}
	}
	if (viewDefinition.postprocess) {
		result = viewDefinition.postprocess(&db, args,result);
	}
	return Result(json,result);

}

QueryBase& QueryBase::group(natural level) {
	groupLevel = level;
	return *this;
}

JSON::ConstValue QueryBase::buildKey(ConstStringT<ConstValue> values) const {
	if (!forceArray && values.length() == 1) {
		return json(values[0]);
	}
	else return json(values);
}

JSON::Container QueryBase::buildRangeKey(ConstStringT<ConstValue> values) const {
	return json(values);
}


QueryBase& QueryBase::reverseOrder() {
	descent = !descent;
	return *this;
}

QueryBase& QueryBase::limit(natural limit) {
	this->maxlimit = limit;
	this->offset = 0;
	this->offset_doc = StringA();
	return *this;
}

QueryBase& QueryBase::limit(natural offset, natural limit) {
	this->maxlimit = limit;
	this->offset = offset;
	this->offset_doc = StringA();
	return *this;
}

QueryBase& QueryBase::limit(ConstStrA docId, natural limit) {
	this->maxlimit = limit;
	this->offset_doc = docId;
	this->offset = 0;
	return *this;
}

QueryBase& QueryBase::updateAfter() {
	staleMode = smUpdateAfter;
	return *this;
}

QueryBase& QueryBase::stale() {
	staleMode = smStale;
	return *this;
}

QueryBase::QueryBase(const QueryBase& other)
	:json(other.json)
{
}


void QueryBase::finishCurrent() const
{
	if (curKeySet.empty())
		return;
	JSON::ConstValue arr = buildKey(curKeySet);
	curKeySet.clear();
	switch (mode) {
		case mdKeys: {
			if (keys == nil) {
				startkey = endkey = nil;
				keys = json.array();
			}
			keys.add(arr);
			return;
		}
		case mdStart: {
			keys = nil;
			startkey = arr;
			return;
		}
		case mdEnd: {
			keys = nil;
			endkey = arr;
			return;
		}
	}
}


Result::Result(const Json &json, ConstValue jsonResult)
	:ConstValue(jsonResult["rows"]),json(json),rdpos(0)
{
	JSON::INode *jtotal = jsonResult->getPtr("total_rows");
	if (jtotal) total = jtotal->getUInt(); else total = this->length();
	JSON::INode *joffset = jsonResult->getPtr("offset");
	if (joffset) offset = joffset->getUInt(); else offset = 0;
}

const ConstValue& Result::getNext() {
	out = (*this)[rdpos++];
	return out;
}

const ConstValue& Result::peek() const {
	out = (*this)[rdpos];
	return out;
}

bool Result::hasItems() const {
	return rdpos < this->length();
}

natural Result::getTotal() const {
	return total;
}

natural Result::getOffset() const {
	return offset;
}


natural Result::getRemain() const {
	return this->length() - rdpos;
}

void Result::rewind() {
	rdpos = 0;
}


Row::Row(const ConstValue& jrow)
	:ConstValue(jrow)
	,key(jrow->getPtr("key"))
	,value(jrow->getPtr("value"))
	,doc(jrow->getPtr("doc"))
	,id(jrow->getPtr("id"))
	,error(jrow->getPtr("error"))
{}

JSON::Value QueryBase::initArgs() {
	if (args==null) args = json.object();
	return args;
}


QueryBase::~QueryBase() {
}

Query::Query(CouchDB &db, const View &view):QueryBase(db.json,view.flags),db(db),viewDefinition(view) {
}

Query::Query(const Query& other):QueryBase(other),db(other.db),viewDefinition(other.viewDefinition) {
}

Query::~Query() {

}

/*Result Result::join(const JSON::Path foreignKey,QueryBase& q,ConstStrA resultName) {


	Container newRows = json.array();
	Container result = json("rows",newRows);


		typedef Map<ConstValue, Container, JsonIsLess> Keys;
		Keys keys;

		while (hasItems()) {
			Row rw = getNext();
			ConstValue frKey = foreignKey(rw.value);
			if (frKey == null) {
				result.add(rw);
			} else {
				Container newObj = json.object();
				newObj.load(rw);
				Container &target = keys(frKey);
				if (target == nil) {
					target = json.array();
				}
				Container newVal;
				if (rw.value == null) {
					newVal = json.array();
					newVal.add(target);
				} else if (rw.value->isArray()) {
					newVal.load(rw.value);
					newVal.add(target);
				} else if (rw.value->isObject() && !resultName.empty()) {
					newVal = json.object();
					newVal.load(rw.value);
					newVal.set(resultName,target);
				} else {
					newVal = json.array();
					newVal.add(rw.value);
					newVal.add(target);
				}
				newObj.set("value",newVal);
			}
		}

	for (auto iter = keys.getFwIter(); iter.hasItems();) {
		q.selectKey(iter.getNext().key);
	}

	Result r = q.exec();

	Keys::Iterator kiter = keys.getFwIter();
	while (r.hasItems()) {
		bool notMatch = true;
		Row rr = r.getNext();
		while (kiter.hasItems() && notMatch) {
			const Keys::KeyValue &kv = kiter.peek();
			CompareResult r = compareJson(kv.key,rr.key);
			if (r == cmpResultEqual) {
				kv.value.add(rr.value);
				notMatch = false;
			} else if (r == cmpResultGreater) {
				Container *c = keys.find(rr.key);
				if (c) c->add(rr.value);
				notMatch = false;
			} else if (r == cmpResultLess) {
				kiter.skip();
			}
		}
		if (notMatch) {
			Container *c = keys.find(rr.key);
			if (c) c->add(rr.value);
		}
	}

	return Result(json,result);
}*/


/*
Result& Result::merge(MergeType mergeType,Result& other) {
}

*/
}

