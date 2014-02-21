#include "all_tests.h"

static char *ok_sample[] = {
		"test/resource/tlv/ok_int-1.tlv",
		"test/resource/tlv/ok_int-2.tlv",
		"test/resource/tlv/ok_int-3.tlv",
		"test/resource/tlv/ok_int-4.tlv",
		"test/resource/tlv/ok_int-5.tlv",
		"test/resource/tlv/ok_int-6.tlv",
		"test/resource/tlv/ok_int-7.tlv",
		"test/resource/tlv/ok_int-8.tlv",
		"test/resource/tlv/ok_nested-1.tlv",
		"test/resource/tlv/ok_nested-2.tlv",
		"test/resource/tlv/ok_nested-3.tlv",
		"test/resource/tlv/ok_nested-4.tlv",
		"test/resource/tlv/ok_nested-5.tlv",
		"test/resource/tlv/ok_nested-6.tlv",
		"test/resource/tlv/ok_nested-7.tlv",
		"test/resource/tlv/ok_nested-8.tlv",
		"test/resource/tlv/ok_str-1.tlv",
		"test/resource/tlv/ok_str-2.tlv",
		"test/resource/tlv/ok_str-3.tlv",
		"test/resource/tlv/ok_str-4.tlv",
		"test/resource/tlv/ok_str-5.tlv",
		"test/resource/tlv/ok_str-6.tlv",
		NULL
};

static char *nok_sample[] = {
		"test/resource/tlv/nok_int-1.tlv",
		"test/resource/tlv/nok_int-2.tlv",
		"test/resource/tlv/nok_int-3.tlv",
		"test/resource/tlv/nok_int-4.tlv",
		"test/resource/tlv/nok_int-5.tlv",
		"test/resource/tlv/nok_str-1.tlv",
		NULL
};

static KSI_CTX *ctx = NULL;

static void initEnv(CuTest* tc) {
	int res;

	res = KSI_CTX_new(&ctx);
	CuAssert(tc, "Failed to init context", res == KSI_OK && ctx != NULL);
}

static void closeEnv(CuTest* tc) {
	KSI_CTX_free(ctx);
	ctx = NULL;
}

static int tlvFromFile(CuTest* tc, char *fileName, KSI_TLV **tlv) {
	int res;
	KSI_RDR *rdr = NULL;

	lprintf("Open TLV file: '%s'\n", fileName);

	res = KSI_RDR_fromFile(ctx, fileName, "rb", &rdr);
	if (res != KSI_OK) goto cleanup;

	res = KSI_TLV_fromReader(rdr, tlv);
	if (res != KSI_OK) goto cleanup;

cleanup:

	KSI_RDR_close(rdr);

	return res;
}

static int parseStructure(KSI_TLV *tlv, int indent) {
	int res;
	uint64_t uint;
	unsigned char *buf;
	KSI_TLV *nested = NULL;

	lprintf("%*sTLV:\n", indent++*4, "");
	lprintf("%*sTLV type: 0x%04x\n",indent*4, "", tlv->type);

	switch (tlv->type) {
		case 0x01:
			/* Parse number */
			lprintf("%*sPayload type: UINT64\n", indent*4, "");
			res = KSI_TLV_getUInt64Value(tlv, &uint);
			if (res != KSI_OK) goto cleanup;
			break;
		case 0x02:
			/* Parse string */
			lprintf("%*sPayload type: STR\n", indent*4, "");
			res = KSI_TLV_getStringValue(tlv, &buf, 0);
			if (res != KSI_OK) goto cleanup;
			break;
		case 0x03:
		case 0x1003:
			/* Parse nested */
			lprintf("%*sPayload type: NESTED\n", indent*4, "");
			while (1) {
				res = KSI_TLV_getNextNestedTLV(tlv, &nested);
				if (res != KSI_OK) goto cleanup;

				if (nested == NULL) break;

				res = parseStructure(nested, indent);
				if (res != KSI_OK) goto cleanup;
			}
			break;
		default:
			res = KSI_INVALID_FORMAT;
			goto cleanup;
	}

cleanup:

	return res;
}

static void TestOkFiles(CuTest* tc) {
	int res;
	int i;

	KSI_TLV *tlv = NULL;

	initEnv(tc);

	while (ok_sample[i] != NULL) {
		CuAssert(tc, "Unable to read valid TLV", tlvFromFile(tc, ok_sample[i++], &tlv) == KSI_OK);

		CuAssert(tc, "Unable to parse valid TLV", parseStructure(tlv, 0) == KSI_OK);

		CuAssert(tc, "Failed to parse structure.", res == KSI_OK);

		KSI_TLV_free(tlv);
		tlv = NULL;
	}

	closeEnv(tc);
}

static void TestNokFiles(CuTest* tc) {
	int res;
	int i;

	KSI_TLV *tlv = NULL;

	initEnv(tc);

	while (nok_sample[i] != NULL) {
		res = tlvFromFile(tc, nok_sample[i++], &tlv);

		if (res == KSI_OK) {
			res = parseStructure(tlv, 0);
		}

		CuAssert(tc, "Parser did not fail with invalid TLV", res != KSI_OK);

		KSI_TLV_free(tlv);
		tlv = NULL;
	}

	closeEnv(tc);
}


CuSuite* KSI_TLV_Sample_GetSuite(void)
{
	CuSuite* suite = CuSuiteNew();

	SUITE_ADD_TEST(suite, TestOkFiles);
	SUITE_ADD_TEST(suite, TestNokFiles);

	return suite;
}
