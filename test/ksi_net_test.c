/*
 * Copyright 2013-2015 Guardtime, Inc.
 *
 * This file is part of the Guardtime client SDK.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES, CONDITIONS, OR OTHER LICENSES OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 * "Guardtime" and "KSI" are trademarks or registered trademarks of
 * Guardtime, Inc., and no license to trademarks is granted; Guardtime
 * reserves and retains all trademark rights.
 */

#include <string.h>
#include <ksi/net.h>
#include <ksi/pkitruststore.h>

#include "all_tests.h"
#include "../src/ksi/ctx_impl.h"
#include "../src/ksi/net_http_impl.h"
#include "../src/ksi/net_uri_impl.h"
#include "../src/ksi/net_tcp_impl.h"
#include "ksi/net_uri.h"

extern KSI_CTX *ctx;

#define TEST_SIGNATURE_FILE "resource/tlv/ok-sig-2014-04-30.1.ksig"

static int mockHeaderCounter = 0;

static unsigned char mockImprint[] ={
		0x01, 0x11, 0xa7, 0x00, 0xb0, 0xc8, 0x06, 0x6c, 0x47, 0xec, 0xba, 0x05, 0xed, 0x37, 0xbc, 0x14, 0xdc,
		0xad, 0xb2, 0x38, 0x55, 0x2d, 0x86, 0xc6, 0x59, 0x34, 0x2d, 0x1d, 0x7e, 0x87, 0xb8, 0x77, 0x2d};

static void preTest(void) {
	ctx->netProvider->requestCount = 0;
}

static void testSigning(CuTest* tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;
	unsigned char *raw = NULL;
	size_t raw_len = 0;
	unsigned char expected[0x1ffff];
	size_t expected_len = 0;
	FILE *f = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_DataHash_fromImprint(ctx, mockImprint, sizeof(mockImprint), &hsh);
	CuAssert(tc, "Unable to create data hash object from raw imprint", res == KSI_OK && hsh != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok-sig-2014-07-01.1-aggr_response.tlv"));

	res = KSI_createSignature(ctx, hsh, &sig);
	CuAssert(tc, "Unable to sign the hash", res == KSI_OK && sig != NULL);

	res = KSI_Signature_serialize(sig, &raw, &raw_len);
	CuAssert(tc, "Unable to serialize signature.", res == KSI_OK && raw != NULL && raw_len > 0);

	f = fopen(getFullResourcePath("resource/tlv/ok-sig-2014-07-01.1.ksig"), "rb");
	CuAssert(tc, "Unable to load sample signature.", f != NULL);

	expected_len = (unsigned)fread(expected, 1, sizeof(expected), f);
	CuAssert(tc, "Failed to read sample", expected_len > 0);

	CuAssert(tc, "Serialized signature length mismatch", expected_len == raw_len);
	CuAssert(tc, "Serialized signature content mismatch.", !memcmp(expected, raw, raw_len));

	if (f != NULL) fclose(f);
	KSI_free(raw);
	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
}

static void testAggreAuthFailure(CuTest* tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_DataHash_fromImprint(ctx, mockImprint, sizeof(mockImprint), &hsh);
	CuAssert(tc, "Unable to create data hash object from raw imprint", res == KSI_OK && hsh != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/aggr_error_pdu.tlv"));

	res = KSI_createSignature(ctx, hsh, &sig);
	CuAssert(tc, "Aggregation should fail with service error.", res == KSI_SERVICE_AUTHENTICATION_FAILURE && sig == NULL);

	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
}

static int mockHeaderCallback(KSI_Header *hdr) {
	int res = KSI_UNKNOWN_ERROR;
	KSI_Integer *msgId = NULL;
	KSI_Integer *instId = NULL;

	++mockHeaderCounter;

	if (hdr == NULL) {
		res = KSI_INVALID_ARGUMENT;
		goto cleanup;
	}

	res = KSI_Header_getInstanceId(hdr, &instId);
	if (res != KSI_OK) goto cleanup;
	if (instId != NULL) {
		res = KSI_INVALID_ARGUMENT;
		KSI_LOG_error(ctx, "Header already contains a instance Id.");
		goto cleanup;
	}

	if (mockHeaderCounter != 1) {
		return KSI_OK;
	}

	res = KSI_Header_getMessageId(hdr, &msgId);
	if (res != KSI_OK) goto cleanup;
	if (msgId != NULL) {
		res = KSI_INVALID_ARGUMENT;
		KSI_LOG_error(ctx, "Header already contains a message Id.");
		goto cleanup;
	}

	res = KSI_Integer_new(ctx, 1337, &instId);
	if (res != KSI_OK) goto cleanup;

	res = KSI_Integer_new(ctx, 5, &msgId);
	if (res != KSI_OK) goto cleanup;

	res = KSI_Header_setMessageId(hdr, msgId);
	if (res != KSI_OK) goto cleanup;
	msgId = NULL;

	res = KSI_Header_setInstanceId(hdr, instId);
	if (res != KSI_OK) goto cleanup;
	instId = NULL;

	res = KSI_OK;

cleanup:

	KSI_Integer_free(instId);
	KSI_Integer_free(msgId);

	return res;
}

static void testAggregationHeader(CuTest* tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_NetworkClient *pr = NULL;
	KSI_AggregationReq *req = NULL;
	KSI_AggregationPdu *pdu = NULL;
	KSI_RequestHandle *handle = NULL;
	KSI_Integer *tmp = NULL;
	KSI_Header *hdr = NULL;
	KSI_Integer *reqId = NULL;
	const unsigned char *raw = NULL;
	size_t raw_len = 0;

	KSI_ERR_clearErrors(ctx);

	mockHeaderCounter = 0;

	res = KSI_CTX_setRequestHeaderCallback(ctx, mockHeaderCallback);
	CuAssert(tc, "Unable to set header callback.", res == KSI_OK);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_DataHash_fromImprint(ctx, mockImprint, sizeof(mockImprint), &hsh);
	CuAssert(tc, "Unable to create data hash object from raw imprint", res == KSI_OK && hsh != NULL);

	res = KSI_AggregationReq_new(ctx, &req);
	CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

	res = KSI_AggregationReq_setRequestHash(req, hsh);
	CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);
	hsh = NULL;

	res = KSI_Integer_new(ctx, 17, &reqId);
	CuAssert(tc, "Unable to create reqId", res == KSI_OK && reqId != NULL);

	res = KSI_AggregationReq_setRequestId(req, reqId);
	CuAssert(tc, "Unable to set request id.", res == KSI_OK);
	reqId = NULL;

	res = KSI_NetworkClient_sendSignRequest(pr, req, &handle);
	CuAssert(tc, "Unable to send request.", res == KSI_OK && handle != NULL);

	KSI_AggregationReq_free(req);
	req = NULL;

	res = KSI_RequestHandle_getRequest(handle, &raw, &raw_len);
	CuAssert(tc, "Unable to get request.", res == KSI_OK && raw != NULL);

	res = KSI_AggregationPdu_parse(ctx, (unsigned char *)raw, raw_len, &pdu);
	CuAssert(tc, "Unable to parse the request pdu.", res == KSI_OK && pdu != NULL);

	res = KSI_AggregationPdu_getHeader(pdu, &hdr);
	CuAssert(tc, "Unable to get header from pdu.", res == KSI_OK && hdr != NULL);

	res = KSI_Header_getMessageId(hdr, &tmp);
	CuAssert(tc, "Unable to get message id from header.", res == KSI_OK && tmp != NULL);
	CuAssert(tc, "Wrong message id.", KSI_Integer_equalsUInt(tmp, 5));
	tmp = NULL;

	res = KSI_Header_getInstanceId(hdr, &tmp);
	CuAssert(tc, "Unable to get instance id from header.", res == KSI_OK && tmp != NULL);
	CuAssert(tc, "Wrong instance id.", KSI_Integer_equalsUInt(tmp, 1337));
	tmp = NULL;

	CuAssert(tc, "Mock header callback not called.", mockHeaderCounter == 1);

	res = KSI_CTX_setRequestHeaderCallback(ctx, NULL);
	CuAssert(tc, "Unable to set NULL as header callback.", res == KSI_OK);

	KSI_Integer_free(reqId);
	KSI_DataHash_free(hsh);
	KSI_AggregationPdu_free(pdu);
	KSI_RequestHandle_free(handle);
}

static void testExtending(CuTest* tc) {
	int res;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;
	unsigned char *serialized = NULL;
	size_t serialized_len = 0;
	unsigned char expected[0x1ffff];
	size_t expected_len = 0;
	FILE *f = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath(TEST_SIGNATURE_FILE), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-extend_response.tlv"));

	res = KSI_extendSignature(ctx, sig, &ext);
	CuAssert(tc, "Unable to extend the signature", res == KSI_OK && ext != NULL);

	res = KSI_Signature_serialize(ext, &serialized, &serialized_len);
	CuAssert(tc, "Unable to serialize extended signature", res == KSI_OK && serialized != NULL && serialized_len > 0);

	/* Read in the expected result */
	f = fopen(getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-extended.ksig"), "rb");
	CuAssert(tc, "Unable to read expected result file", f != NULL);
	expected_len = (unsigned)fread(expected, 1, sizeof(expected), f);
	fclose(f);

	CuAssert(tc, "Expected result length mismatch", expected_len == serialized_len);
	CuAssert(tc, "Unexpected extended signature.", !KSITest_memcmp(expected, serialized, expected_len));

	KSI_free(serialized);

	KSI_Signature_free(sig);
	KSI_Signature_free(ext);

}

static void testExtendTo(CuTest* tc) {
	int res;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;
	unsigned char *serialized = NULL;
	size_t serialized_len = 0;
	unsigned char expected[0x1ffff];
	size_t expected_len = 0;
	FILE *f = NULL;
	KSI_Integer *to = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath(TEST_SIGNATURE_FILE), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-extend_response.tlv"));

	KSI_Integer_new(ctx, 1400112000, &to);

	res = KSI_Signature_extendTo(sig, ctx, to, &ext);
	CuAssert(tc, "Unable to extend the signature", res == KSI_OK && ext != NULL);

	res = KSI_Signature_serialize(ext, &serialized, &serialized_len);
	CuAssert(tc, "Unable to serialize extended signature", res == KSI_OK && serialized != NULL && serialized_len > 0);

	/* Read in the expected result */
	f = fopen(getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-extended_1400112000.ksig"), "rb");
	CuAssert(tc, "Unable to read expected result file", f != NULL);
	expected_len = (unsigned)fread(expected, 1, sizeof(expected), f);
	fclose(f);

	CuAssert(tc, "Expected result length mismatch", expected_len == serialized_len);
	CuAssert(tc, "Unexpected extended signature.", !KSITest_memcmp(expected, serialized, expected_len));

	KSI_free(serialized);

	KSI_Integer_free(to);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);

}

static void testExtenderWrongData(CuTest* tc) {
	int res;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;
	KSI_Integer *to = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath(TEST_SIGNATURE_FILE), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-extend_response.tlv"));

	/* Create a random date that is different from the response. */
	KSI_Integer_new(ctx, 1400112222, &to);

	res = KSI_Signature_extendTo(sig, ctx, to, &ext);
	CuAssert(tc, "Wrong answer from extender should not be tolerated.", res != KSI_OK && ext == NULL);

	KSI_Integer_free(to);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);

}

static void testExtendInvalidSignature(CuTest* tc) {
	int res;
	KSI_Signature *sig = NULL;
	KSI_Signature *ext = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath("resource/tlv/nok-sig-wrong-aggre-time.tlv"), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/nok-sig-wrong-aggre-time-extend_response.tlv"));

	res = KSI_Signature_extendTo(sig, ctx, NULL, &ext);
	CuAssert(tc, "It should not be possible to extend this signature.", res == KSI_EXTEND_WRONG_CAL_CHAIN && ext == NULL);

	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
}

static void testExtAuthFailure(CuTest* tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;
	KSI_Signature *ext = NULL;
	KSI_PKITruststore *pki = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_CTX_getPKITruststore(ctx, &pki);
	CuAssert(tc, "Unable to get PKI Truststore", res == KSI_OK && pki != NULL);

	res = KSI_PKITruststore_addLookupFile(pki, getFullResourcePath("resource/tlv/mock.crt"));
	CuAssert(tc, "Unable to add test certificate to truststore.", res == KSI_OK);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath(TEST_SIGNATURE_FILE), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ext_error_pdu.tlv"));

	res = KSI_extendSignature(ctx, sig, &ext);
	CuAssert(tc, "Extend should fail with service error.", res == KSI_SERVICE_AUTHENTICATION_FAILURE && ext == NULL);


	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);

}

static void testExtendingWithoutPublication(CuTest* tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;
	KSI_Signature *ext = NULL;
	unsigned char *serialized = NULL;
	size_t serialized_len = 0;
	unsigned char expected[0x1ffff];
	size_t expected_len = 0;
	FILE *f = NULL;
	KSI_PKITruststore *pki = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_CTX_getPKITruststore(ctx, &pki);
	CuAssert(tc, "Unable to get PKI Truststore", res == KSI_OK && pki != NULL);

	res = KSI_PKITruststore_addLookupFile(pki, getFullResourcePath("resource/tlv/mock.crt"));
	CuAssert(tc, "Unable to add test certificate to truststore.", res == KSI_OK);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath(TEST_SIGNATURE_FILE), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-extend_response.tlv"));

	res = KSI_Signature_extend(sig, ctx, NULL, &ext);
	CuAssert(tc, "Unable to extend the signature to the head", res == KSI_OK && ext != NULL);

	res = KSI_Signature_serialize(ext, &serialized, &serialized_len);
	CuAssert(tc, "Unable to serialize extended signature", res == KSI_OK && serialized != NULL && serialized_len > 0);
	KSI_LOG_logBlob(ctx, KSI_LOG_DEBUG, "Signature extended to head", serialized, serialized_len);

	/* Read in the expected result */
	f = fopen(getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-head.ksig"), "rb");
	CuAssert(tc, "Unable to read expected result file", f != NULL);
	expected_len = (unsigned)fread(expected, 1, sizeof(expected), f);
	fclose(f);


	CuAssert(tc, "Expected result length mismatch", expected_len == serialized_len);
	CuAssert(tc, "Unexpected extended signature.", !memcmp(expected, serialized, expected_len));

	KSI_free(serialized);

	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);

}

static void testExtendingToNULL(CuTest* tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;
	KSI_Signature *ext = NULL;
	unsigned char *serialized = NULL;
	size_t serialized_len = 0;
	unsigned char expected[0x1ffff];
	size_t expected_len = 0;
	FILE *f = NULL;
	KSI_PKITruststore *pki = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_CTX_getPKITruststore(ctx, &pki);
	CuAssert(tc, "Unable to get PKI Truststore", res == KSI_OK && pki != NULL);

	res = KSI_PKITruststore_addLookupFile(pki, getFullResourcePath("resource/tlv/mock.crt"));
	CuAssert(tc, "Unable to add test certificate to truststore.", res == KSI_OK);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath(TEST_SIGNATURE_FILE), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-extend_response.tlv"));

	res = KSI_Signature_extendTo(sig, ctx, NULL, &ext);
	CuAssert(tc, "Unable to extend the signature to the head", res == KSI_OK && ext != NULL);

	res = KSI_Signature_serialize(ext, &serialized, &serialized_len);
	CuAssert(tc, "Unable to serialize extended signature", res == KSI_OK && serialized != NULL && serialized_len > 0);
	KSI_LOG_logBlob(ctx, KSI_LOG_DEBUG, "Signature extended to head", serialized, serialized_len);

	/* Read in the expected result */
	f = fopen(getFullResourcePath("resource/tlv/ok-sig-2014-04-30.1-head.ksig"), "rb");
	CuAssert(tc, "Unable to read expected result file", f != NULL);
	expected_len = (unsigned)fread(expected, 1, sizeof(expected), f);
	fclose(f);


	CuAssert(tc, "Expected result length mismatch", expected_len == serialized_len);
	CuAssert(tc, "Unexpected extended signature.", !memcmp(expected, serialized, expected_len));

	KSI_free(serialized);

	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);

}

static void testSigningInvalidResponse(CuTest* tc){
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_DataHash_fromImprint(ctx, mockImprint, sizeof(mockImprint), &hsh);
	CuAssert(tc, "Unable to create data hash object from raw imprint", res == KSI_OK && hsh != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/nok_aggr_response_missing_header.tlv"));
	res = KSI_createSignature(ctx, hsh, &sig);
	CuAssert(tc, "Signature should not be created with invalid aggregation response", res == KSI_INVALID_FORMAT && sig == NULL);

	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
}

static void testSigningErrorResponse(CuTest *tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_DataHash_fromImprint(ctx, mockImprint, sizeof(mockImprint), &hsh);
	CuAssert(tc, "Unable to create data hash object from raw imprint", res == KSI_OK && hsh != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok_aggr_err_response-1.tlv"));
	res = KSI_createSignature(ctx, hsh, &sig);
	CuAssert(tc, "Signature should not be created due to server error.", res == KSI_SERVICE_INVALID_PAYLOAD && sig == NULL);

	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
}

static void testExtendingErrorResponse(CuTest *tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;
	KSI_NetworkClient *pr = NULL;
	KSI_Signature *ext = NULL;
	KSI_PKITruststore *pki = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_CTX_getPKITruststore(ctx, &pki);
	CuAssert(tc, "Unable to get PKI Truststore", res == KSI_OK && pki != NULL);

	res = KSI_PKITruststore_addLookupFile(pki, getFullResourcePath("resource/tlv/mock.crt"));
	CuAssert(tc, "Unable to add test certificate to truststore.", res == KSI_OK);

	res = KSI_NET_MOCK_new(ctx, &pr);
	CuAssert(tc, "Unable to create mock network provider.", res == KSI_OK);

	res = KSI_CTX_setNetworkProvider(ctx, pr);
	CuAssert(tc, "Unable to set network provider.", res == KSI_OK);

	res = KSI_Signature_fromFile(ctx, getFullResourcePath(TEST_SIGNATURE_FILE), &sig);
	CuAssert(tc, "Unable to load signature from file.", res == KSI_OK && sig != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok_extend_err_response-1.tlv"));

	res = KSI_Signature_extend(sig, ctx, NULL, &ext);
	CuAssert(tc, "Extend should fail with server error", res == KSI_SERVICE_INVALID_PAYLOAD && ext == NULL);

	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
	KSI_Signature_free(ext);
}

static void testUrlSplit(CuTest *tc) {
	struct {
		int res;
		const char *uri;
		const char *expSchema;
		const char *expHost;
		const char *expPath;
		const unsigned expPort;
	} testData[] = {
			{KSI_OK, "ksi://guardtime.com:12345", "ksi", "guardtime.com", NULL, 12345},
			{KSI_OK, "ksi+http://guardtime.com", "ksi+http", "guardtime.com", NULL, 0},
			{KSI_OK, "http:///toto", "http", NULL, "/toto", 0 },
			{KSI_INVALID_FORMAT, "guardtime.com:80",  NULL, "guardtime.com", NULL, 0 },
			{KSI_INVALID_FORMAT, "guardtime.com", NULL, NULL, NULL, 0},
			{-1, NULL, NULL, NULL, NULL, 0 }
	};
	int i;
	for (i = 0; testData[i].res >= 0; i++) {
		char *host = NULL;
		char *schema = NULL;
		char *path = NULL;
		unsigned port = 0;
		int res;

		KSI_LOG_debug(ctx, "%s\n", testData[i].uri);
		res = KSI_UriSplitBasic(testData[i].uri, &schema, &host, &port, &path);
		KSI_LOG_debug(ctx, "schema=%s, host=%s, port=%u, path=%s\n", schema, host, port, path);
		CuAssert(tc, "KSI_UriSplitBasic did not return expected status code.", res == testData[i].res);
		if (res == KSI_OK) {
			CuAssertStrEquals_Msg(tc, "KSI_UriSplitBasic did not return expected schema", testData[i].expSchema, schema);
			CuAssertStrEquals_Msg(tc, "KSI_UriSplitBasic did not return expected host", testData[i].expHost, host);
			CuAssertStrEquals_Msg(tc, "KSI_UriSplitBasic did not return expected path", testData[i].expPath, path);
			CuAssert(tc, "KSI_UriSplitBasic did not return expected port", testData[i].expPort == port);
		}
		KSI_free(schema);
		KSI_free(host);
		KSI_free(path);
	}

}

void assert_isHttpClientSetCorrectly(CuTest *tc, KSI_NetworkClient *client,
		const char *a_url, const char *a_host, int a_port, const char *a_user, const char *a_key,
		const char *e_url, const char *e_host, int e_port, const char *e_user, const char *e_key){
	KSI_UriClient *uri = client->impl;
	KSI_NetworkClient *http = uri->httpClient;
	KSI_NetEndpoint *endp_aggr = http->aggregator;
	KSI_NetEndpoint *endp_ext = http->extender;
	struct HttpClient_Endpoint_st *endp_aggr_impl = endp_aggr->implCtx;
	struct HttpClient_Endpoint_st *endp_ext_impl = endp_ext->implCtx;

	CuAssert(tc, "Http client is not set.", http != NULL);
	CuAssert(tc, "Http client is not set as aggregator and extender service.",
			(void*)http == (void*)(uri->pAggregationClient) &&
			(void*)http == (void*)(uri->pExtendClient));

	CuAssert(tc, "Http aggregator url mismatch.", strcmp(endp_aggr_impl->url, strstr(a_url, "ksi+") == a_url ? a_url+4 : a_url) == 0 ||
			(strstr(a_url, "ksi://") == a_url && strstr(endp_aggr_impl->url, "http://") == endp_aggr_impl->url && strcmp(endp_aggr_impl->url+7, a_url+6) == 0));
	CuAssert(tc, "Http aggregator key mismatch.", strcmp(endp_aggr->ksi_pass, a_key) == 0);
	CuAssert(tc, "Http aggregator user mismatch.", strcmp(endp_aggr->ksi_user, a_user) == 0);
	CuAssert(tc, "Http extender url mismatch.", strcmp(endp_ext_impl->url, strstr(e_url, "ksi+") == e_url ? e_url+4 : e_url) == 0 ||
			(strstr(e_url, "ksi://") == e_url && strstr(endp_ext_impl->url, "http://") == endp_ext_impl->url && strcmp(endp_ext_impl->url+7, e_url+6) == 0));
	CuAssert(tc, "Http extender key mismatch.", strcmp(endp_ext->ksi_pass, e_key) == 0);
	CuAssert(tc, "Http extender user mismatch.", strcmp(endp_ext->ksi_user, e_user) == 0);
}

void assert_isTcpClientSetCorrectly(CuTest *tc, KSI_NetworkClient *client,
		const char *a_url, const char *a_host, int a_port, const char *a_user, const char *a_key,
		const char *e_url, const char *e_host, int e_port, const char *e_user, const char *e_key){
	KSI_UriClient *uri = client->impl;
	KSI_NetworkClient *tcp = uri->tcpClient;
	KSI_NetEndpoint *endp_aggr = tcp->aggregator;
	KSI_NetEndpoint *endp_ext = tcp->extender;
	struct TcpClient_Endpoint_st *endp_aggr_impl = endp_aggr->implCtx;
	struct TcpClient_Endpoint_st *endp_ext_impl = endp_ext->implCtx;

	CuAssert(tc, "Tcp client is not set (NULL).", tcp != NULL);
	CuAssert(tc, "Tcp client is not set as aggregator and extender service.",
			(void*)tcp == (void*)(uri->pAggregationClient) &&
			(void*)tcp == (void*)(uri->pExtendClient));

	CuAssert(tc, "Tcp aggregator host mismatch.", strcmp(endp_aggr_impl->host, a_host) == 0);
	CuAssert(tc, "Tcp aggregator port mismatch.", endp_aggr_impl->port == a_port);
	CuAssert(tc, "Tcp aggregator key mismatch.", strcmp(endp_aggr->ksi_pass, a_key) == 0);
	CuAssert(tc, "Tcp aggregator user mismatch.", strcmp(endp_aggr->ksi_user, a_user) == 0);
	CuAssert(tc, "Tcp extender url mismatch.", strcmp(endp_ext_impl->host, e_host) == 0);
	CuAssert(tc, "Tcp extender host mismatch.", endp_ext_impl->port == e_port);
	CuAssert(tc, "Tcp extender key mismatch.", strcmp(endp_ext->ksi_pass, e_key) == 0);
	CuAssert(tc, "Tcp extender user mismatch.", strcmp(endp_ext->ksi_user, e_user) == 0);
}

static void smartServiceSetterSchemeTest(CuTest *tc, KSI_CTX *ctx, const char *scheme,
		void (*assertClient)(CuTest*, KSI_NetworkClient*, const char*, const char*, int, const char*, const char*, const char*, const char*, int, const char*, const char*)){
	int res;
	KSI_NetworkClient *client = NULL;

	char *scheme_aggre_host = "aggre.com";
	int scheme_aggre_port = 3331;
	char *scheme_aggre_user = "tcp_aggre_user";
	char *scheme_aggre_key = "tcp_aggre_key";
	char *scheme_ext_host = "ext.com";
	int scheme_ext_port = 8011;
	char *scheme_ext_user = "tcp_ext_user";
	char *scheme_ext_key = "tcp_ext_key";
	char scheme_aggre_url[1024];
	char scheme_ext_url[1024];
	size_t len;

	len = KSI_snprintf(scheme_aggre_url, sizeof(scheme_aggre_url), "%saggre.com:3331/", scheme);
	CuAssert(tc, "Unable to generate aggregator url.", len != 0);
	len = KSI_snprintf(scheme_ext_url, sizeof(scheme_ext_url), "%sext.com:8011/", scheme);
	CuAssert(tc, "Unable to generate extender url.", len != 0);

	client = ctx->netProvider;
	CuAssert(tc, "KSI_CTX has no network provider.", client != NULL);
	CuAssert(tc, "KSI_CTX network provider is not initial.", ctx->isCustomNetProvider == 0);

	/*Testing client*/

	res = KSI_CTX_setAggregator(ctx, scheme_aggre_url, scheme_aggre_user, scheme_aggre_key);
	CuAssert(tc, "Unable to set aggregator.", res == KSI_OK);

	res = KSI_CTX_setExtender(ctx, scheme_ext_url, scheme_ext_user, scheme_ext_key);
	CuAssert(tc, "Unable to set extender.", res == KSI_OK);

	assertClient(tc, client,
			scheme_aggre_url, scheme_aggre_host, scheme_aggre_port, scheme_aggre_user, scheme_aggre_key,
			scheme_ext_url, scheme_ext_host, scheme_ext_port, scheme_ext_user, scheme_ext_key);

	return;
}


static void testSmartServiceSetters(CuTest *tc) {

	int res;
	KSI_CTX *ctx = NULL;
	KSI_UriClient *client = NULL;

	res = KSI_CTX_new(&ctx);
	CuAssert(tc, "KSI_CTX_init did not return KSI_OK.", res == KSI_OK);

	client = (KSI_UriClient*)ctx->netProvider;
	CuAssert(tc, "KSI_CTX has no network provider.", client != NULL);
	CuAssert(tc, "KSI_CTX network provider is not initial.", ctx->isCustomNetProvider == 0);

	smartServiceSetterSchemeTest(tc, ctx, "ksi://", assert_isHttpClientSetCorrectly);
	smartServiceSetterSchemeTest(tc, ctx, "http://", assert_isHttpClientSetCorrectly);
	smartServiceSetterSchemeTest(tc, ctx, "https://", assert_isHttpClientSetCorrectly);
	smartServiceSetterSchemeTest(tc, ctx, "ksi+https://", assert_isHttpClientSetCorrectly);
	smartServiceSetterSchemeTest(tc, ctx, "ksi+http://", assert_isHttpClientSetCorrectly);
	smartServiceSetterSchemeTest(tc, ctx, "ksi+tcp://", assert_isTcpClientSetCorrectly);

	KSI_CTX_free(ctx);
}

static void testLocalAggregationSigning(CuTest* tc) {
	int res;
	KSI_DataHash *hsh = NULL;
	KSI_Signature *sig = NULL;

	KSI_ERR_clearErrors(ctx);

	res = KSI_DataHash_fromImprint(ctx, mockImprint, sizeof(mockImprint), &hsh);
	CuAssert(tc, "Unable to create data hash object from raw imprint", res == KSI_OK && hsh != NULL);

	KSITest_setFileMockResponse(tc, getFullResourcePath("resource/tlv/ok-local_aggr_lvl4_resp.tlv"));

	res = KSI_Signature_createAggregated(ctx, hsh, 4, &sig);
	CuAssert(tc, "Unable to sign the hash", res == KSI_OK && sig != NULL);

	res = KSI_Signature_verify(sig, NULL);
	CuAssert(tc, "Signature should not be verifiable without local aggregation level.", res == KSI_VERIFICATION_FAILURE);

	res = KSI_Signature_verifyAggregated(sig, NULL, 4);
	CuAssert(tc, "Locally aggregated signature was not verifiable.", res == KSI_OK);

	KSI_DataHash_free(hsh);
	KSI_Signature_free(sig);
}

static 	const char *validUri[] = {
		"ksi://localhost",
		"ksi://localhost/",
		"ksi://localhost/a",
		"ksi://localhost/a.txt",
		"ksi://localhost/?key=value",
		"ksi://localhost?key=value",
		"ksi://localhost?key=value#fragment",
		"ksi://localhost/#fragment",
		"ksi+http://localhost",
		"ksi://localhost:12345",
		"ksi+http://localhost:1234/",
		"http://u:p@127.0.0.1:80",
		"http://u:p@127.0.0.1:80/",
		"http://u:p@127.0.0.1:80/test",
		"http://u:p@127.0.0.1:80/test/",
		"http://u:p@127.0.0.1:80/test/a",
		"http://u:p@127.0.0.1:80/test/b/",
		"http://u:p@127.0.0.1:80/test/c//",
		"http://u:p@127.0.0.1:80/test/c/test.file",
		"http://u:p@127.0.0.1:80/test/c?a=test&b=test&c=test",
		"http://u:p@127.0.0.1:80/test/c.txt?a=test&b=test&c=test",
		"http://u:p@127.0.0.1:80/test/c.txt?a=test&b=test&c=test#fragment1",
		"http://u:p@127.0.0.1:80/test/c.txt#fragment1",
		NULL
};

static void testUriSpiltAndCompose(CuTest* tc) {
	int res;
	KSI_NetworkClient *tmp = NULL;
	size_t i = 0;
	const char *uri = NULL;

	char error[0xffff];
	char new_uri[0xffff];
	char *scheme = NULL;
	char *user = NULL;
	char *pass = NULL;
	char *host = NULL;
	unsigned port = 0;
	char *path = NULL;
	char *query = NULL;
	char *fragment = NULL;

	res = KSI_AbstractNetworkClient_new(ctx, &tmp);
	CuAssert(tc, "Unable to create abstract network provider.", res == KSI_OK && tmp != NULL);


	while ((uri = validUri[i++]) != NULL) {
		scheme = NULL;
		user = NULL;
		pass = NULL;
		host = NULL;
		port = 0;
		path = NULL;
		query = NULL;
		fragment = NULL;
		error[0] = '\0';
		new_uri[0] = '\0';

		res = tmp->uriSplit(uri, &scheme, &user, &pass, &host, &port, &path, &query, &fragment);
		if (res != KSI_OK) {
			KSI_snprintf(error, sizeof(error), "Unable to split uri '%s'.", uri);
			CuAssert(tc, error, 0);
		}

		res = tmp->uriCompose(scheme, user, pass, host, port, path, query, fragment, new_uri, sizeof(new_uri));
		if (res != KSI_OK) {
			KSI_snprintf(error, sizeof(error), "Unable to compose uri '%s'.", uri);
			CuAssert(tc, error, 0);
		}

		if (strcmp(uri, new_uri) != 0) {
			KSI_snprintf(error, sizeof(error), "New uri is '%s', but expected '%s'.", new_uri, uri);
			CuAssert(tc, error, 0);
		}


		KSI_free(scheme);
		KSI_free(user);
		KSI_free(pass);
		KSI_free(path);
		KSI_free(host);
		KSI_free(query);
		KSI_free(fragment);
	}

	KSI_NetworkClient_free(tmp);
}


CuSuite* KSITest_NET_getSuite(void) {
	CuSuite* suite = CuSuiteNew();

	suite->preTest = preTest;

	SUITE_ADD_TEST(suite, testSigning);
	SUITE_ADD_TEST(suite, testAggreAuthFailure);
	SUITE_ADD_TEST(suite, testExtending);
	SUITE_ADD_TEST(suite, testExtendTo);
	SUITE_ADD_TEST(suite, testExtenderWrongData);
	SUITE_ADD_TEST(suite, testExtAuthFailure);
	SUITE_ADD_TEST(suite, testExtendingWithoutPublication);
	SUITE_ADD_TEST(suite, testExtendingToNULL);
	SUITE_ADD_TEST(suite, testSigningInvalidResponse);
	SUITE_ADD_TEST(suite, testAggregationHeader);
	SUITE_ADD_TEST(suite, testSigningErrorResponse);
	SUITE_ADD_TEST(suite, testExtendingErrorResponse);
	SUITE_ADD_TEST(suite, testUrlSplit);
	SUITE_ADD_TEST(suite, testSmartServiceSetters);
	SUITE_ADD_TEST(suite, testUriSpiltAndCompose);
	SUITE_ADD_TEST(suite, testLocalAggregationSigning);
	SUITE_ADD_TEST(suite, testExtendInvalidSignature);

	return suite;
}

