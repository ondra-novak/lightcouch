/*
 * attachment.h
 *
 *  Created on: 19. 6. 2016
 *      Author: ondra
 */

#ifndef LIBS_LIGHTCOUCH_SRC_LIGHTCOUCH_ATTACHMENT_H_
#define LIBS_LIGHTCOUCH_SRC_LIGHTCOUCH_ATTACHMENT_H_
#include "lightspeed/base/containers/string.h"
#include "lightspeed/utils/json.h"

#include "string.h"

#include "object.h"
namespace LightCouch {

using namespace LightSpeed;

///Contains attachment data as reference
/** use this class to refer some binary data (with proper content type). Data
 * are not stored with the object, you must not destroy refered object
 */
class AttachmentDataRef: public ConstBin {
public:
	///Constructor
	/**
	 *
	 * @param data reference to binary data
	 * @param contenType content type
	 */
	AttachmentDataRef(const ConstBin data, const StringA &contentType)
		:ConstBin(data),contentType(contentType) {}
	const StringA contentType;

	///Converts data to base64 string.
	StringA toBase64() const;

	Value toInline(const Json &json) const;

};

///Contains attachment data
/** use this class to store some binary data with proper content type. Data
 * are stored with the object. This may result to some extra memory allocation.
 *
 * Because StringB is used, you can prepare the binary data into this object. If you
 * pass StringB as argument then no copying is made, object is shared instead.
 */
class AttachmentData: public AttachmentDataRef {
public:

	///Constructor
	/**
	 * @param data binary data
	 * @param contentType content type
	 */
	AttachmentData(const StringB &data, const StringA &contentType)
		:AttachmentDataRef(data,contentType),bindata(data) {}
	///Constructor from JSON value
	/**
	 * @param attachment value contains attachment from a document. It required that
	 * attachment is inlined, you cannot construct object from a stub.
	 */
	AttachmentData(const ConstValue &attachment);

	///Creates attachment from base64 string
	/**
	 * @param base64 base64 string
	 * @param contentType contenr type
	 * @return attachment object
	 */
	static AttachmentData fromBase64(ConstStrA base64, ConstStrA contentType);

private:
	StringB bindata;
};

}


#endif /* LIBS_LIGHTCOUCH_SRC_LIGHTCOUCH_ATTACHMENT_H_ */
