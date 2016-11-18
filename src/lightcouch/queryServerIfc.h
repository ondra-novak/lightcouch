/*
 * queryServerIfc.h
 *
 *  Created on: 16. 11. 2016
 *      Author: ondra
 */

#ifndef LIGHTCOUCH_QUERYSERVERIFC_H_
#define LIGHTCOUCH_QUERYSERVERIFC_H_
#include "document.h"


#pragma once

#include "reducerow.h"

namespace LightCouch {

using namespace LightSpeed;

///Emit function - object is passed to the map() function and function should call it to emit result to the view
class IEmitFn {
public:
	///Emit current document to the view without key and value.
	/** You can use this function to build sets of document with some feature.
	 Both key and value is set to null
	 */
	virtual void operator()() = 0;

	///Emit current document under specified key
	/**
	 * @param key key value. You can emit multiple documents under single key. You can use
	 * multicollum keys (emit as an array). You can also use object as key, however be careful,
	 * because LightCouch doesn't maintain order of fields. Fields are always ordered.
	 */
	virtual void operator()(const Value &key) = 0;
	///Emit current document under specified key with a value
	/**
	 * @param key key value. You can emit multiple documents under single key. You can use
	 * multicollum keys (emit as an array). You can also use object as key, however be careful,
	 * because LightCouch doesn't maintain order of fields. Fields are always ordered.
	 * @param value value.
	 */
	virtual void operator()(const Value &key, const Value &value) = 0;
};

///Base class for every QueryServer's handler
class QueryServerHandler {
public:


	///Each view must have version.
	/** Everytime map or reduce function is modified, the version should be updated.
	 * Changing version triggers two events. First the cauchdb regenerates the view. Second,
	 * it also restarts the query server.
	 *
	 * @return version. Value is considered static, it should not change over time.
	 */
	virtual natural version() const = 0;

	virtual ~QueryServerHandler() {}

};

///Abstract map-reduce function (base class)
/** Because map-reduce function is complex, you should define this function as an object
 * which implements all three phases of map-reduce feature.
 *
 * If you need to implement only map function without reduce, use AbstractMapFn instead
 */
class AbstractViewBase: public QueryServerHandler {
public:


	enum ReduceMode {
		///View has no reduce
		rmNone,
		///View has defined reduce() and rereduce() functions
		rmFunction,
		///View uses build-in _count function
		rmCount,
		///View uses build-in _sum function
		rmSum,
		///View uses build-in _stats function
		rmStats
	};

	///perform map operation
	/**
	 * @param doc whole document to map
	 * @param emit function to use to emit result
	 */
	virtual void map(const Document &doc, IEmitFn &emit) = 0;
	///function returns true, because object supports reduce
	virtual ReduceMode reduceMode() const {return rmFunction;}
	///called to reduce rows
	/**
	 * @param rows array of rows to reduce
	 * @return reduced result
	 */
	virtual Value reduce(const RowsWithKeys &rows) = 0;

	///Called when rereduce is required
	/**
	 * Function should reduce already reduced rows. CouchDB can optimize reduce by reducing
	 * per-partes. This function is called to reduce already reduced results.
	 *
	 * @param values array of reduced result
	 * @return reduced result
	 *
	 * @note function can be called by multiple times with already reduced result generated by
	 * previous run.
	 */
	virtual Value rereduce(const ReducedRows &values) = 0;

};

///Abstract map-reduce function
/** Because map-reduce function is complex, you should define this function as an object
 * which implements all three phases of map-reduce feature.
 *
 * If you need to implement only map function without reduce, use AbstractViewMapOnly instead
 *
 * Template class required an argument
 * @tparam ver version number. Everytime map or reduce is changed, version must be changed as well
 * to enforce reindexing of the view. However, without changing the version, the updated
 * code will apply for newly added items
 *
 */

template<natural ver>
class AbstractView: public AbstractViewBase {
public:
	virtual natural version() const override {return ver;}
};


///Abstract map function
/** Map only view with buildin reduce function (performed by couchdb itself)
 *
 *@tparam ver version - see AbstractView
 *@tparam mode Specify reduce mode. It can be rmSum, rmCount or rmStats. You can also specify
 *        rmNone which is implemented in class AbstractViewMapOnly. You should not use
 *        rmFunction, which is default for AbstractView itself
 */
template<natural ver, AbstractViewBase::ReduceMode mode>
class AbstractViewBuildin: public AbstractView<ver> {
public:
	typedef typename AbstractView<ver>::ReduceMode ReduceMode;

	virtual ReduceMode reduceMode() const override {return mode;}
	virtual Value reduce(const RowsWithKeys &) override {return null;}
	virtual Value rereduce(const ReducedRows &) override {return null;}
};

///Abstract map function
/** Map only view (without reduce)
 *
 * Template class required an argument
 * @tparam ver version number. Everytime map or reduce is changed, version must be changed as well
 * to enforce reindexing of the view. However, without changing the version, the updated
 * code will apply for newly added items
 */
template<natural ver>
class AbstractViewMapOnly: public AbstractViewBuildin<ver, AbstractViewBase::rmNone> {
public:
};


///List function context
class IListContext {
public:
	virtual ~IListContext() {}
	///Receive next row from view
	/**
	 * @return a valid row data, or null if none available. If result is not null, it
	 * can be converted to Row object
	 *
	 * @note function also flushes any data sent by send() function. The response
	 * object in the context is also sent, so any modification in it will not applied.
	 */
	virtual Value getRow() = 0;
	///send text to output
	virtual void send(StrView text) = 0;
	///send json to output
	virtual void send(Value jsonValue) = 0;
	///send anything convertible to string
	template<typename Str>
	void send(const Str &txt) {send(StrView(txt));}
	///retrieve view header
	virtual Value getViewHeader() const = 0;
	///Initializes output
	/**
	 * @param headerObject header object, see couchdb documentation about lists
	 *
	 * Function must be called before first getRow() is called, otherwise it is ignored
	 * If called by multiple-times, before getRow(), header fields are merged
	 */
	virtual void start(Value headerObject) = 0;
};

///List function (base class)
/** Use AbstractList */
class AbstractListBase: public QueryServerHandler {
public:
	///List function
	/**
	 * @param list contains interface to enumerate rows
	 *
	 * Works similar as JS version. You receive IListContext through the variable list. You
	 * can call method of this interface inside of the function. The function getRow returns
	 * next row from the view. Object can be converted to Row object.
	 *
	 * According to couchdb's query protocol, the function send() will emit its output
	 * during the getRow is processed. Sends after last row has been received will
	 * be processed on function exit. You don't need to process all rows.
	 */
	virtual void run(IListContext &list, Value request) = 0;
	virtual ~AbstractListBase() {}
};

///show function (base class)
/** Use AbstractShow */
class AbstractShowBase: public QueryServerHandler {
public:
	///Show function
	/**
	 * @param doc document to show
	 * @param request request (see CouchDB documentation)
	 * @return response object
	 */

	virtual Value run(const Document &doc, Value request) = 0;
	virtual ~AbstractShowBase() {}
};

///update function (base class)
/** Use AbstractUpdate */
class AbstractUpdateFnBase: public QueryServerHandler {
public:
	///Update function
	/**
	 * @param doc document to change. Change the document (make it dirty) and the document will be stored
	 *   (note it can be null)
	 * @param request request
	 * @return response object
	 */
	virtual Value run(Document &doc, Value request) = 0;
	virtual ~AbstractUpdateFnBase() {}
};

///Filter function (base class)
/** Use AbstractFilter */
class AbstractFilterBase: public QueryServerHandler {
public:
	///Filter function
	/**
	 * @param doc document
	 * @param request request object
	 * @retval true allow document
	 * @retval false disallow document
	 */
	virtual bool run(const Document &doc, Value request) = 0;
	virtual ~AbstractFilterBase() {}
};

template<natural ver> class AbstractList: public AbstractListBase {
	virtual natural version() const {return ver;}
};
template<natural ver> class AbstractShow: public AbstractShowBase {
	virtual natural version() const {return ver;}
};
template<natural ver> class AbstractUpdateFn: public AbstractUpdateFnBase {
	virtual natural version() const {return ver;}
};
template<natural ver> class AbstractFilter: public AbstractFilterBase {
	virtual natural version() const {return ver;}
};

class QueryServerError: public Exception {
public:
	LIGHTSPEED_EXCEPTIONFINAL;

	QueryServerError(const ProgramLocation &loc, StringA type, StringA explain)
		:Exception(loc),type(type),explain(explain) {}
	const StringA &getType() const;
	const StringA &getExplain() const;
protected:
	StringA type, explain;
	void message(ExceptionMsg &msg) const;

};

class VersionMistmatch: public Exception {
public:
	LIGHTSPEED_EXCEPTIONFINAL;

	VersionMistmatch(const ProgramLocation &loc):Exception(loc) {}
protected:
	void message(ExceptionMsg &msg) const;

};


}


#endif /* LIGHTCOUCH_QUERYSERVERIFC_H_ */