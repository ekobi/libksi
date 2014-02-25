#include <stdio.h>
#include <string.h>

#include "cutest/CuTest.h"
#include "../src/ksi_internal.h"
#include "../src/ksi_tlv.h"

static void TestTlvInitOwnMem(CuTest* tc) {
	KSI_CTX *ctx = NULL;
	KSI_TLV *tlv = NULL;
	int res;

	KSI_CTX_new(&ctx);

	res = KSI_TLV_new(ctx, NULL, 0, &tlv);
	CuAssert(tc, "Failed to create TLV.", res == KSI_OK);
	CuAssert(tc, "Created TLV is NULL.", tlv != NULL);

	CuAssert(tc, "TLV buffer is null.", tlv->buffer != NULL);

	CuAssert(tc, "TLV encoding is wrong.", tlv->payloadType == KSI_TLV_PAYLOAD_RAW);

	CuAssert(tc, "TLV raw does not point to buffer.", tlv->payload.rawVal.ptr == tlv->buffer);

	KSI_TLV_free(tlv);
	KSI_CTX_free(ctx);
}

static void TestTlvInitExtMem(CuTest* tc) {
	KSI_CTX *ctx = NULL;
	KSI_TLV *tlv = NULL;
	int res;

	unsigned char tmp[0xff];

	KSI_CTX_new(&ctx);

	res = KSI_TLV_new(ctx, tmp, sizeof(tmp), &tlv);
	CuAssert(tc, "Failed to create TLV.", res == KSI_OK);
	CuAssert(tc, "Created TLV is NULL.", tlv != NULL);

	CuAssert(tc, "TLV buffer is not null.", tlv->buffer == NULL);

	CuAssert(tc, "TLV encoding is wrong.", tlv->payloadType == KSI_TLV_PAYLOAD_RAW);

	CuAssert(tc, "TLV raw does not point to external memory.", tlv->payload.rawVal.ptr == tmp);

	KSI_TLV_free(tlv);
	KSI_CTX_free(ctx);
}

static void TestTlv8FromReader(CuTest* tc) {
	int res;
	/* TLV type = 7, length = 21 */
	char raw[] = "\x07\x15THIS IS A TLV CONTENT";

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	CuAssert(tc, "Failed to create reader.", res == KSI_OK && rdr != NULL);

	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Failed to create TLV from reader.", res == KSI_OK && tlv != NULL);

	CuAssertIntEquals_Msg(tc, "TLV length", 21, tlv->payload.rawVal.length);
	CuAssertIntEquals_Msg(tc, "TLV type", 7, tlv->type);

	CuAssert(tc, "TLV content differs", !memcmp(tlv->payload.rawVal.ptr, raw + 2, tlv->payload.rawVal.length));

	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlv8getRawValueCopy(CuTest* tc) {
	int res;
	/* TLV type = 7, length = 21 */
	unsigned char raw[] = "\x07\x15THIS IS A TLV CONTENT";
	unsigned char *tmp = NULL;
	int tmp_len = sizeof(tmp);


	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Failed to create TLV from reader.", res == KSI_OK && tlv != NULL);

	res = KSI_TLV_getRawValue(tlv, &tmp, &tmp_len, 1);
	CuAssert(tc, "Failed to retrieve raw value.", res == KSI_OK);

	CuAssertIntEquals_Msg(tc, "TLV payload length", 21, tmp_len);

	/* Make sure the pointer does not point to the original. */
	CuAssert(tc, "Data pointer points to original value.", tmp != tlv->payload.rawVal.ptr);


	CuAssert(tc, "TLV content differs", !memcmp(tmp, raw + 2, tmp_len));

	/* Change the value. */
	++*tmp;

	CuAssert(tc, "Original value changed.", memcmp(tmp, tlv->payload.rawVal.ptr, tmp_len));

	KSI_free(tmp);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlv8getRawValueSharedMem(CuTest* tc) {
	int res;
	/* TLV type = 7, length = 21 */
	unsigned char raw[] = "\x07\x15THIS IS A TLV CONTENT";
	unsigned char *tmp = NULL;
	int tmp_len = sizeof(tmp);


	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Failed to create TLV from reader.", res == KSI_OK && tlv != NULL);

	res = KSI_TLV_getRawValue(tlv, &tmp, &tmp_len, 0);
	CuAssert(tc, "Failed to retrieve raw value.", res == KSI_OK);

	CuAssertIntEquals_Msg(tc, "TLV payload length", 21, tmp_len);

	/* Make sure the pointer *does* point to the original. */
	CuAssert(tc, "Data pointer points to original value.", tmp == tlv->payload.rawVal.ptr);

	/* Change the value. */
	++*tmp;

	CuAssert(tc, "Original value did not changed.", !memcmp(tmp, tlv->payload.rawVal.ptr, tmp_len));

	KSI_nofree(tmp);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlv16FromReader(CuTest* tc) {
	int res;
	/* TLV16 type = 0x2aa, length = 21 */
	char raw[] = "\x82\xaa\x00\x15THIS IS A TLV CONTENT";

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);

	CuAssert(tc, "Failed to create reader.", res == KSI_OK && rdr != NULL);

	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Failed to create TLV from reader.", res == KSI_OK && tlv != NULL);

	CuAssertIntEquals_Msg(tc, "TLV length", 21, tlv->payload.rawVal.length);
	CuAssertIntEquals_Msg(tc, "TLV type", 0x2aa, tlv->type);

	CuAssert(tc, "TLV content differs", !memcmp(tlv->payload.rawVal.ptr, raw + 4, tlv->payload.rawVal.length));

	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlvGetUint64(CuTest* tc) {
	int res;
	/* TLV type = 1a, length = 8 */
	unsigned char raw[] = {0x1a, 0x08, 0xca, 0xfe, 0xba, 0xbe, 0xca, 0xfe, 0xfa, 0xce};
	uint64_t value;

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	KSI_RDR_fromMem(ctx, raw, sizeof(raw), 1, &rdr);
	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Failed to create TLV from reader.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_INT);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getUInt64Value(tlv, &value);
	CuAssert(tc, "Parsing uint64 failed.", res == KSI_OK);

	CuAssert(tc, "Parsed value is not correct.", value == 0xcafebabecafeface);

	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);

}

static void TestTlvGetUint64Overflow(CuTest* tc) {
	int res;
	/* TLV type = 1a, length = 8 */
	unsigned char raw[] = {0x1a, 0x09, 0xca, 0xfe, 0xba, 0xbe, 0xca, 0xfe, 0xfa, 0xce, 0xee};
	uint64_t value;

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw), 1, &rdr);
	CuAssert(tc, "Failed to create reader from memory buffer.", res == KSI_OK && rdr != NULL);


	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Failed to create TLV from reader.", res == KSI_OK && tlv != NULL);

	res = KSI_TLV_getUInt64Value(tlv, &value);
	CuAssert(tc, "Parsing uint64 with overflow should not succeed.", res != KSI_OK);

	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlvGetStringValue(CuTest* tc) {
	int res;
	/* TLV16 type = 0x2aa, length = 21 */
	char raw[] = "\x82\xaa\x00\x0alore ipsum";
	char *str = NULL;

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	res = KSI_CTX_new(&ctx);
	CuAssert(tc, "Unable to create context", res == KSI_OK && ctx != NULL);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	CuAssert(tc, "Unable to create reader.", res == KSI_OK && rdr != NULL);
	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Unable to create TLV from reader.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_STR);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);


	res = KSI_TLV_getStringValue(tlv, &str, 0);
	CuAssert(tc, "Failed to get string value from tlv.", res == KSI_OK && str != NULL);
	CuAssert(tc, "TLV payload type not string.", tlv->payloadType == KSI_TLV_PAYLOAD_STR);

	CuAssert(tc, "Returned value does not point to original value.", str == tlv->payload.rawVal.ptr);

	CuAssert(tc, "Returned string is not what was expected", !strcmp("lore ipsum", str));

	KSI_nofree(str);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlvGetStringValueCopy(CuTest* tc) {
	int res;
	/* TLV16 type = 0x2aa, length = 21 */
	char raw[] = "\x82\xaa\x00\x0alore ipsum";
	char *str = NULL;

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	CuAssert(tc, "Unable to create reader.", res == KSI_OK && rdr != NULL);

	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Unable to create TLV from reader.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_STR);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getStringValue(tlv, &str, 1);
	CuAssert(tc, "Failed to get string value from tlv.", res == KSI_OK && str != NULL);

	CuAssert(tc, "Returned value *does* point to original value.", str != (char *)tlv->payload.rawVal.ptr);

	CuAssert(tc, "Returned string is not what was expected", !strcmp("lore ipsum", str));

	KSI_free(str);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlvGetNextNested(CuTest* tc) {
	int res;
	/* TLV16 type = 0x2aa, length = 21 */
	unsigned char raw[] = "\x01\x1f" "\x07\x15" "THIS IS A TLV CONTENT" "\x7\x06" "\xca\xff\xff\xff\xff\xfe";
	char *str = NULL;

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;
	KSI_TLV *nested = NULL;
	uint64_t uint;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	CuAssert(tc, "Unable to create reader.", res == KSI_OK && rdr != NULL);

	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Unable to read TLV.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_TLV);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Unable to read nested TLV", res == KSI_OK && nested != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(nested, KSI_TLV_PAYLOAD_STR);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getStringValue(nested, &str, 1);
	CuAssert(tc, "Unable to read string from nested TLV", res == KSI_OK && str != NULL);
	CuAssert(tc, "Unexpected string from nested TLV.", !strcmp("THIS IS A TLV CONTENT", str));

	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Unable to read nested TLV", res == KSI_OK && nested != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(nested, KSI_TLV_PAYLOAD_INT);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getUInt64Value(nested, &uint);
	CuAssert(tc, "Unable to read uint from nested TLV", res == KSI_OK);
	CuAssert(tc, "Unexpected uint value from nested TLV", 0xcafffffffffe == uint);

	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Reading nested TLV failed after reading last TLV.", res == KSI_OK);
	CuAssert(tc, "Nested element should have been NULL", nested == NULL);


	KSI_free(str);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);

}

static void TestTlvGetNextNestedSharedMemory(CuTest* tc) {
	int res;
	/* TLV16 type = 0x2aa, length = 21 */
	unsigned char raw[] = "\x01\x1f" "\x07\x15" "THIS IS A TLV CONTENT" "\x7\x06" "\xca\xff\xff\xff\xff\xfe";
	char *str = NULL;

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;
	KSI_TLV *nested = NULL;
	uint64_t uint;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	CuAssert(tc, "Unable to create reader.", res == KSI_OK && rdr != NULL);

	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Unable to read TLV.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_TLV);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Unable to read nested TLV", res == KSI_OK && nested != NULL);
	CuAssert(tc, "Nested TLV buffer is not NULL", nested->buffer == NULL);
	CuAssert(tc, "Nested TLV memory area out of parent buffer.",
			nested->payload.rawVal.ptr > tlv->buffer && nested->payload.rawVal.ptr <= tlv->buffer + tlv->buffer_size );

	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Unable to read nested TLV", res == KSI_OK && nested != NULL);
	CuAssert(tc, "Nested TLV buffer is not NULL", nested->buffer == NULL);
	CuAssert(tc, "Nested TLV memory area out of parent buffer.",
			nested->payload.rawVal.ptr > tlv->buffer && nested->payload.rawVal.ptr <= tlv->buffer + tlv->buffer_size );

	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Reading nested TLV failed after reading last TLV.", res == KSI_OK);
	CuAssert(tc, "Nested element should have been NULL", nested == NULL);


	KSI_free(str);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);

}

static void TestTlvSerializeString(CuTest* tc) {
	int res;
	/* TLV16 type = 0x2aa, length = 21 */
	char raw[] = "\x82\xaa\x00\x0alore ipsum";
	char *str = NULL;
	int buf_len = 0xffff;
	unsigned char buf[buf_len];


	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	res = KSI_CTX_new(&ctx);
	CuAssert(tc, "Unable to create context", res == KSI_OK && ctx != NULL);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	CuAssert(tc, "Unable to create reader.", res == KSI_OK && rdr != NULL);
	res = KSI_TLV_fromReader(rdr, &tlv);
	KSI_ERR_statusDump(ctx, stdout);
	CuAssert(tc, "Unable to create TLV from reader.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_STR);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getStringValue(tlv, &str, 0);
	CuAssert(tc, "Failed to get string value from tlv.", res == KSI_OK && str != NULL);

	res = KSI_TLV_serialize(tlv, buf, &buf_len);
	CuAssert(tc, "Failed to serialize string TLV", res == KSI_OK);
	CuAssertIntEquals_Msg(tc, "Size of serialized TLV", sizeof(raw) - 1, buf_len);

	CuAssert(tc, "Serialized TLV does not match original", !memcmp(raw, buf, buf_len));

	KSI_nofree(str);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlvSerializeUint(CuTest* tc) {
	int res;
	/* TLV type = 1a, length = 8 */
	unsigned char raw[] = {0x1a, 0x08, 0xca, 0xfe, 0xba, 0xbe, 0xca, 0xfe, 0xfa, 0xce};
	uint64_t value;
	int buf_len = 0xffff;
	unsigned char buf[buf_len];


	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw), 1, &rdr);
	CuAssert(tc, "Failed to create reader from memory buffer.", res == KSI_OK && rdr != NULL);


	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Failed to create TLV from reader.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_INT);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getUInt64Value(tlv, &value);
	CuAssert(tc, "Parsing uint64 with overflow should not succeed.", res == KSI_OK);

	res = KSI_TLV_serialize(tlv, buf, &buf_len);
	CuAssert(tc, "Failed to serialize string value of tlv.", res == KSI_OK);
	CuAssertIntEquals_Msg(tc, "Size of serialized TLV", sizeof(raw), buf_len);

	CuAssert(tc, "Serialized value does not match", !memcmp(raw, buf, buf_len));

	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);
}

static void TestTlvSerializeNested(CuTest* tc) {
	int res;
	unsigned char raw[] = "\x01\x1f" "\x07\x15" "THIS IS A TLV CONTENT" "\x7\x06" "\xca\xff\xff\xff\xff\xfe";
	char *str = NULL;
	int buf_len = 0xffff;
	unsigned char buf[buf_len];

	KSI_CTX *ctx = NULL;
	KSI_RDR *rdr = NULL;
	KSI_TLV *tlv = NULL;
	KSI_TLV *nested = NULL;
	uint64_t uint;

	KSI_CTX_new(&ctx);

	res = KSI_RDR_fromMem(ctx, raw, sizeof(raw) - 1, 1, &rdr);
	CuAssert(tc, "Unable to create reader.", res == KSI_OK && rdr != NULL);

	res = KSI_TLV_fromReader(rdr, &tlv);
	CuAssert(tc, "Unable to read TLV.", res == KSI_OK && tlv != NULL);

	/* Cast payload type */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_TLV);
	CuAssert(tc, "TLV cast failed", res == KSI_OK);

	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Unable to read nested TLV", res == KSI_OK && nested != NULL);
	CuAssert(tc, "Nested TLV buffer is not NULL", nested->buffer == NULL);

	res = KSI_TLV_serialize(tlv, buf, &buf_len);

	CuAssert(tc, "Failed to serialize nested values of tlv.", res == KSI_OK);
	CuAssertIntEquals_Msg(tc, "Size of serialized TLV", sizeof(raw) - 1, buf_len);

	CuAssert(tc, "Serialized value does not match original", !memcmp(raw, buf, buf_len));


	KSI_free(str);
	KSI_TLV_free(tlv);
	KSI_RDR_close(rdr);
	KSI_CTX_free(ctx);

}

static void TestTlvRequireCast(CuTest* tc) {
	int res;
	KSI_CTX *ctx = NULL;
	KSI_TLV *tlv = NULL;
	KSI_TLV *nested = NULL;

	unsigned char *ptr = NULL;
	int len;
	uint64_t uintval;

	res = KSI_CTX_new(&ctx);
	CuAssert(tc, "Unable to create context", res == KSI_OK && ctx != NULL);

	unsigned char raw[] = "\x07\x06QWERTY";
	res = KSI_TLV_fromBlob(ctx, raw, sizeof(raw) - 1, &tlv);
	CuAssert(tc, "Unable to create TLV", res == KSI_OK && tlv != NULL);

	/* Should not fail */
	res = KSI_TLV_getRawValue(tlv, &ptr, &len, 0);
	CuAssert(tc, "Failed to get raw value without a cast", res == KSI_OK && ptr != NULL);
	ptr = NULL;

	/* Should fail. */
	res = KSI_TLV_getStringValue(tlv, &ptr, 0);
	CuAssert(tc, "Got string value without a cast", res != KSI_OK);
	ptr = NULL;

	/* Should fail. */
	res = KSI_TLV_getUInt64Value(tlv, &uintval);
	CuAssert(tc, "Got uint value without a cast", res != KSI_OK);
	ptr = NULL;

	/* Should fail. */
	res = KSI_TLV_getNextNestedTLV(tlv, &nested);
	CuAssert(tc, "Got nested TLV without a cast", res != KSI_OK);
	ptr = NULL;


	/* Cast as string */
	res = KSI_TLV_cast(tlv, KSI_TLV_PAYLOAD_STR);
	CuAssert(tc, "Failed to cast TLV to nested string.", res == KSI_OK);

	/* After cast, this should not fail */
	res = KSI_TLV_getStringValue(tlv, &ptr, 0);
	CuAssert(tc, "Failed to get string value after a cast to string", res == KSI_OK);
	ptr = NULL;

	/* Should fail */
	res = KSI_TLV_getRawValue(tlv, &ptr, &len, 0);
	CuAssert(tc, "Got raw value after a cast to string", res != KSI_OK);
	ptr = NULL;

	KSI_TLV_free(tlv);
	KSI_nofree(nested);
	KSI_CTX_free(ctx);
}

CuSuite* KSI_TLV_GetSuite(void)
{
	CuSuite* suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, TestTlvInitOwnMem);
	SUITE_ADD_TEST(suite, TestTlvInitExtMem);
	SUITE_ADD_TEST(suite, TestTlv8FromReader);
	SUITE_ADD_TEST(suite, TestTlv8getRawValueCopy);
	SUITE_ADD_TEST(suite, TestTlv8getRawValueSharedMem);
	SUITE_ADD_TEST(suite, TestTlv16FromReader);
	SUITE_ADD_TEST(suite, TestTlvGetUint64);
	SUITE_ADD_TEST(suite, TestTlvGetUint64Overflow);
	SUITE_ADD_TEST(suite, TestTlvGetStringValue);
	SUITE_ADD_TEST(suite, TestTlvGetStringValueCopy);
	SUITE_ADD_TEST(suite, TestTlvGetNextNested);
	SUITE_ADD_TEST(suite, TestTlvGetNextNestedSharedMemory);
	SUITE_ADD_TEST(suite, TestTlvSerializeString);
	SUITE_ADD_TEST(suite, TestTlvSerializeUint);
	SUITE_ADD_TEST(suite, TestTlvSerializeNested);
	SUITE_ADD_TEST(suite, TestTlvRequireCast);

	return suite;
}
