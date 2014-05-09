#include <stdlib.h>
#include "ksi_common.h"

#ifndef KSI_TLV_TEMPLATE_H_
#define KSI_TLV_TEMPLATE_H_

#ifdef __cplusplus
extern "C" {
#endif
	typedef struct KSI_TlvTemplate_st KSI_TlvTemplate;

	typedef int (*getter_t)(const void *, void **);
	typedef int (*setter_t)(void *, void *);
	typedef int (*cb_decode_t)(KSI_CTX *ctx, KSI_TLV *, void *, getter_t, setter_t);

	typedef int (*cb_encode_t)(KSI_CTX *ctx, KSI_TLV *, const void *, const KSI_TlvTemplate *);
	struct KSI_TlvTemplate_st {
		int type;
		int tag;
		int isNonCritical;
		int isForward;
		/* Getter and setter for the internal value. */
		getter_t getValue;
		setter_t setValue;

		/* Constructor and destructor for the internal value. */
		int (*construct)(KSI_CTX *, void **);
		void (*destruct)(void *);



		const KSI_TlvTemplate *subTemplate;
		/* List functions */
		int (*listAppend)(void *, void *);
		/* Can this element be added multiple times (usefull with collections). */
		int multiple;
		int (*listNew)(KSI_CTX *, void **);
		void (*listFree)(void *);

		/* Callbacks */
		cb_encode_t callbackEncode;
		cb_decode_t callbackDecode;
	};

#define KSI_TLV_TEMPLATE(name) name##_template
#define KSI_IMPORT_TLV_TEMPLATE(name) extern const KSI_TlvTemplate KSI_TLV_TEMPLATE(name)[];

	#define KSI_TLV_TEMPLATE_INTEGER 				1
	#define KSI_TLV_TEMPLATE_OCTET_STRING 			2
	#define KSI_TLV_TEMPLATE_UTF8_STRING 			3
	#define KSI_TLV_TEMPLATE_IMPRINT 				4
	#define KSI_TLV_TEMPLATE_COMPOSITE				5
	#define KSI_TLV_TEMPLATE_CALLBACK				7
	#define KSI_TLV_TEMPLATE_NATIVE_INT				8

	#define KSI_TLV_FULL_TEMPLATE_DEF(typ, tg, nc, fw, gttr, sttr, constr, destr, subTmpl, list_append, mul, list_new, list_free, cbEnc, cbDec) { typ, tg, nc, fw, (getter_t)gttr, (setter_t)sttr, (int (*)(KSI_CTX *, void **)) constr, (void (*)(void *)) destr, subTmpl, (int (*)(void *, void *))list_append, mul, (int (*)(KSI_CTX *, void **)) list_new, (void (*)(void *)) list_free, (cb_encode_t)cbEnc, (cb_decode_t)cbDec},
	#define KSI_TLV_PRIMITIVE_TEMPLATE_DEF(type, tag, isNonCritical, isForward, getter, setter) KSI_TLV_FULL_TEMPLATE_DEF(type, tag, isNonCritical, isForward, getter, setter, NULL, NULL, NULL, NULL, 0, NULL, NULL, NULL, NULL)

	#define KSI_DEFINE_TLV_TEMPLATE(name)	const KSI_TlvTemplate name##_template[] = {
	#define KSI_TLV_INTEGER(tag, isNonCritical, isForward, getter, setter) 					KSI_TLV_PRIMITIVE_TEMPLATE_DEF(KSI_TLV_TEMPLATE_INTEGER, tag, isNonCritical, isForward, getter, setter)
	#define KSI_TLV_NATIVE_INT(tag, isNonCritical, isForward, getter, setter) 				KSI_TLV_PRIMITIVE_TEMPLATE_DEF(KSI_TLV_TEMPLATE_NATIVE_INT, tag, isNonCritical, isForward, getter, setter)

	#define KSI_TLV_OCTET_STRING(tag, isNonCritical, isForward, getter, setter) 			KSI_TLV_PRIMITIVE_TEMPLATE_DEF(KSI_TLV_TEMPLATE_OCTET_STRING, tag, isNonCritical, isForward, getter, setter)
	#define KSI_TLV_OCTET_STRING_LIST(tag, isNonCritical, isForward, getter, setter) 		KSI_TLV_FULL_TEMPLATE_DEF(KSI_TLV_TEMPLATE_OCTET_STRING, tag, isNonCritical, isForward, getter, setter, NULL, NULL, NULL, KSI_OctetStringList_append, 1, KSI_OctetStringList_new, KSI_OctetStringList_free, NULL, NULL)

	#define KSI_TLV_UTF8_STRING(tag, isNonCritical, isForward, getter, setter) 				KSI_TLV_PRIMITIVE_TEMPLATE_DEF(KSI_TLV_TEMPLATE_UTF8_STRING, tag, isNonCritical, isForward, getter, setter)
	#define KSI_TLV_UTF8_STRING_LIST(tag, isNonCritical, isForward, getter, setter) 		KSI_TLV_FULL_TEMPLATE_DEF(KSI_TLV_TEMPLATE_UTF8_STRING, tag, isNonCritical, isForward, getter, setter, NULL, NULL, NULL, KSI_Utf8StringList_append, 1, KSI_Utf8StringList_new, KSI_Utf8StringList_free, NULL, NULL)

	#define KSI_TLV_IMPRINT(tag, isNonCritical, isForward, getter, setter) 					KSI_TLV_PRIMITIVE_TEMPLATE_DEF(KSI_TLV_TEMPLATE_IMPRINT, tag, isNonCritical, isForward, getter, setter)

	#define KSI_TLV_COMPOSITE(tag, isNonCritical, isForward, getter, setter, sub)			KSI_TLV_FULL_TEMPLATE_DEF(KSI_TLV_TEMPLATE_COMPOSITE, tag, isNonCritical, isForward, getter, setter, sub##_new, sub##_free, sub##_template, NULL, 0,  NULL, NULL, NULL, NULL)
	#define KSI_TLV_COMPOSITE_LIST(tag, isNonCritical, isForward, getter, setter, sub) 		KSI_TLV_FULL_TEMPLATE_DEF(KSI_TLV_TEMPLATE_COMPOSITE, tag, isNonCritical, isForward, getter, setter, sub##_new, sub##_free, sub##_template, sub##List_append, 1, sub##List_new, sub##List_free, NULL, NULL)

	#define KSI_TLV_CALLBACK(tag, isNonCritical, isForward, getter, setter, encode, decode)	KSI_TLV_FULL_TEMPLATE_DEF(KSI_TLV_TEMPLATE_CALLBACK, tag, isNonCritical, isForward, getter, setter, NULL, NULL, NULL, NULL, 1, NULL, NULL, encode, decode)
	#define KSI_END_TLV_TEMPLATE { -1, 0, 0, 0, NULL, NULL}};


	/**
	 * Given a TLV object, template and a initialized target payload, this function evaluates the payload objects
	 * with the data from the TLV.
	 *
	 * \param[in]		ctx			KSI context.
	 * \param[in, out]	payload		Preinitialized empty object to be evaluated with the TLV values.
	 * \param[in]		tlv			TLV value which has the structure represented in \c template.
	 * \param[in]		template	Template of the TLV expected structure.
	 * \param[in, out]	reminder	List of TLV's that did not match the template on the first level. Can be NULL, in which case
	 * 								an error code is returned if an unknown non-critical TLV is encountered.
	 *
	 * \return status code (\c KSI_OK, when operation succeeded, otherwise an
	 * error code).
	 */
	int KSI_TlvTemplate_extract(KSI_CTX *ctx, void *payload, KSI_TLV *tlv, const KSI_TlvTemplate *template, KSI_LIST(KSI_TLV) *reminder);

	/**
	 *
	 */
	int KSI_TlvTemplate_extractGenerator(KSI_CTX *ctx, void *payload, void *generatorCtx, const KSI_TlvTemplate *template, KSI_LIST(KSI_TLV) *reminder, int (*generator)(void *, KSI_TLV **));

	/**
	 *
	 */
	int KSI_TlvTemplate_construct(KSI_CTX *ctx, KSI_TLV *tlv, const void *payload, const KSI_TlvTemplate *template);

#ifdef __cplusplus
}
#endif

#endif /* KSI_TLV_TEMPLATE_H_ */