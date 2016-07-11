/*
 * exception.cpp
 *
 *  Created on: 9. 3. 2016
 *      Author: ondra
 */

#include "exception.h"

namespace LightCouch {

RequestError::RequestError(const ProgramLocation& loc, StringA url, natural code,
		StringA message, JSON::Value extraInfo)
	:HttpStatusException(loc,url,code,message), extraInfo(extraInfo)
{
}

RequestError::~RequestError() throw () {
}

void RequestError::message(ExceptionMsg& msg) const {
	JSON::PFactory f = JSON::create();
	ConstStrA details;
	if (this->extraInfo != null) {
		details = f->toString(*extraInfo);
	}
	msg("CouchDB error: url=%1, status=%2, message=%3, details=%4")
		<< this->url
		<< this->status
		<< this->statusMsg
		<< details;
}

const char *DocumentNotEditedException::msgText = "Document %1 is not edited. You have to call edit() first";
const char *DocumentNotEditedException::msgNone = "<n/a>";



}
