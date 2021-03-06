/*
 * Copyright 2013-2017 Guardtime, Inc.
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
#ifdef _WIN32
#  include <windows.h>
#  define sleep_ms(x) Sleep((x))
#else
#  include <unistd.h>
#  define sleep_ms(x) usleep((x)*1000)
#endif

#include "cutest/CuTest.h"
#include "all_integration_tests.h"

#include <ksi/net_uri.h>
#include <ksi/net_http.h>
#include <ksi/net_tcp.h>
#include <ksi/net_async.h>
#include <ksi/net.h>
#include <ksi/hash.h>

extern KSI_CTX *ctx;
extern KSITest_Conf conf;

static const char *TEST_REQUESTS[] = {
	"Guardtime", "Keyless", "Signature", "Infrastructure", "(KSI)",
	"is an", "industrial", "scale", "blockchain", "platform",
	"that", "cryptographically", "ensures", "data", "integrity",
	"and", "proves", "time", "of", "existence",
	NULL
};
static const size_t NOF_TEST_REQUESTS = (sizeof(TEST_REQUESTS) / sizeof(TEST_REQUESTS[0])) - 1;

static void verifyOption(CuTest* tc, KSI_AsyncService *s, int opt, size_t defVal, size_t newVal) {
	int res;
	size_t optVal = 0;

	res = KSI_AsyncService_getOption(s, opt, (void *)&optVal);
	CuAssert(tc, "Async service option value mismatch.", res == KSI_OK && optVal == defVal);

	res = KSI_AsyncService_setOption(s, opt, (void *)newVal);
	CuAssert(tc, "Unable to set async service option.", res == KSI_OK);

	res = KSI_AsyncService_getOption(s, opt, (void *)&optVal);
	CuAssert(tc, "Async service option value mismatch.", res == KSI_OK && optVal == newVal);
}

static void asyncSigning_verifyOptions(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	verifyOption(tc, as, KSI_ASYNC_OPT_CON_TIMEOUT, 10, 15);
	verifyOption(tc, as, KSI_ASYNC_OPT_RCV_TIMEOUT, 10, 15);
	verifyOption(tc, as, KSI_ASYNC_OPT_SND_TIMEOUT, 10, 15);
	verifyOption(tc, as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, 1, 15);
	verifyOption(tc, as, KSI_ASYNC_OPT_MAX_REQUEST_COUNT, 1, 15);

	KSI_AsyncService_free(as);
}

void Test_AsyncSingningService_verifyOptions_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_verifyOptions(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSingningService_verifyOptions_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_verifyOptions(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_verifyCacheSizeOption(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	size_t optVal = 0;

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void *)10);
	CuAssert(tc, "Unable to set async service option.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void *)10);
	CuAssert(tc, "Unable to set async service option.", res == KSI_OK);

	res = KSI_AsyncService_getOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void *)&optVal);
	CuAssert(tc, "Async service option value mismatch.", res == KSI_OK && optVal == 10);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void *)(optVal - 1));
	CuAssert(tc, "Unable to set async service option.", res == KSI_INVALID_ARGUMENT);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void *)(optVal + 1));
	CuAssert(tc, "Unable to set async service option.", res == KSI_OK);

	KSI_AsyncService_free(as);
}

void Test_AsyncSingningService_verifyCacheSizeOption_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_verifyCacheSizeOption(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSingningService_verifyCacheSizeOption_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_verifyCacheSizeOption(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_loop_getResponse(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	time_t startTime;
	const char **p_req = NULL;
	size_t onHold = 0;
	size_t received = 0;

	KSI_LOG_debug(ctx, "%s: START (%s)", __FUNCTION__, url);
	KSI_ERR_clearErrors(ctx);
	startTime = time(NULL);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_MAX_REQUEST_COUNT, (void *)(1 << 3));
	CuAssert(tc, "Unable to set maximum request count.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void*)(NOF_TEST_REQUESTS));
	CuAssert(tc, "Unable to set request cache size.", res == KSI_OK);

	p_req = TEST_REQUESTS;
	do {
		KSI_AsyncHandle *respHandle = NULL;
		int state = KSI_ASYNC_STATE_UNDEFINED;

		if (*p_req != NULL) {
			size_t pendingCount = 0;
			KSI_AsyncHandle *reqHandle = NULL;
			KSI_DataHash *hsh = NULL;
			KSI_AggregationReq *req = NULL;

			KSI_LOG_debug(ctx, "%s: REQUEST (\"%s\").", __FUNCTION__, *p_req);

			res = KSI_DataHash_create(ctx, *p_req, strlen(*p_req), KSI_HASHALG_SHA2_256, &hsh);
			CuAssert(tc, "Unable to create data hash from string.", res == KSI_OK && hsh != NULL);

			res = KSI_AggregationReq_new(ctx, &req);
			CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

			res = KSI_AggregationReq_setRequestHash(req, hsh);
			CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

			res = KSI_AsyncAggregationHandle_new(ctx, req, &reqHandle);
			CuAssert(tc, "Unable to create async request.", res == KSI_OK && reqHandle != NULL);

			res = KSI_AsyncHandle_setRequestCtx(reqHandle, (void*)KSI_DataHash_ref(hsh), (void (*)(void*))KSI_DataHash_free);
			CuAssert(tc, "Unable to set request context.", res == KSI_OK);

			res = KSI_AsyncService_addRequest(as, reqHandle);
			CuAssert(tc, "Unable to add request.", res == KSI_OK);
			p_req++;

			res = KSI_AsyncService_getPendingCount(as, &pendingCount);
			CuAssert(tc, "Unable to get pending count.", res == KSI_OK);
			CuAssert(tc, "Pending count must be >0.", pendingCount > 0);
		}

		KSI_LOG_debug(ctx, "%s: RUN.", __FUNCTION__);

		res = KSI_AsyncService_run(as, &respHandle, &onHold);
		CuAssert(tc, "Failed to run async service.", res == KSI_OK);

		if (respHandle == NULL) {
			if (*p_req == NULL) {
				/* There is nothing to be sent. */
				/* Wait for a while to avoid busy loop. */
				KSI_LOG_debug(ctx, "%s: SLEEP.", __FUNCTION__);
				sleep_ms(50);
			}
			continue;
		}

		res = KSI_AsyncHandle_getState(respHandle, &state);
		CuAssert(tc, "Unable to get request state.", res == KSI_OK && state != KSI_ASYNC_STATE_UNDEFINED);

		switch (state) {
			case KSI_ASYNC_STATE_RESPONSE_RECEIVED: {
					KSI_DataHash *reqCtx = NULL;
					KSI_DataHash *inpHsh = NULL;
					KSI_DataHash *docHsh = NULL;
					KSI_DataHash *reqHsh = NULL;
					KSI_AggregationResp *resp = NULL;
					KSI_AggregationReq *req = NULL;
					KSI_AggregationHashChainList *aggrChainList = NULL;
					KSI_AggregationHashChain *chain = NULL;
					KSI_Signature *signature = NULL;
					int error = 0;
					long errorExt = 0;
					KSI_Utf8String *msg = NULL;

					KSI_LOG_debug(ctx, "%s: RESPONSE.", __FUNCTION__);

					res = KSI_AsyncHandle_getAggregationResp(respHandle, &resp);
					CuAssert(tc, "Failed to get aggregation response.", res == KSI_OK && resp != NULL);

					res = KSI_AsyncHandle_getRequestCtx(respHandle, (const void**)&reqCtx);
					CuAssert(tc, "Unable to get request context.", res == KSI_OK && reqCtx != NULL);

					res = KSI_AggregationResp_getAggregationChainList(resp, &aggrChainList);
					CuAssert(tc, "Unable to get aggregation chain list.", res == KSI_OK && aggrChainList != NULL);
					CuAssert(tc, "Unable to get aggregation chain list is emty.", KSI_AggregationHashChainList_length(aggrChainList) > 0);

					res = KSI_AggregationHashChainList_elementAt(aggrChainList, 0, &chain);
					CuAssert(tc, "Unable to get aggregation chain.", res == KSI_OK && chain != NULL);

					res = KSI_AggregationHashChain_getInputHash(chain, &inpHsh);
					CuAssert(tc, "Unable to chain input hash.", res == KSI_OK && inpHsh != NULL);

					CuAssert(tc, "Request context data mismatch.", KSI_DataHash_equals(reqCtx, inpHsh));

					res = KSI_AsyncHandle_getSignature(respHandle, &signature);
					CuAssert(tc, "Unable to extract signature.", res == KSI_OK && signature != NULL);

					res = KSI_Signature_getDocumentHash(signature, &docHsh);
					CuAssert(tc, "Unable to get document hash.", res == KSI_OK && docHsh != NULL);

					res = KSI_AsyncHandle_getAggregationReq(respHandle, &req);
					CuAssert(tc, "Unable to get aggregation request.", res == KSI_OK && req != NULL);

					res = KSI_AggregationReq_getRequestHash(req, &reqHsh);
					CuAssert(tc, "Unable to get request hash.", res == KSI_OK && reqHsh != NULL);

					CuAssert(tc, "Request hash mismatch.", KSI_DataHash_equals(reqHsh, inpHsh));
					CuAssert(tc, "Document hash mismatch.", KSI_DataHash_equals(docHsh, inpHsh));

					res = KSI_AsyncHandle_getError(respHandle, &error);
					CuAssert(tc, "There should be no error.", res == KSI_OK && error == KSI_OK);

					res = KSI_AsyncHandle_getExtError(respHandle, &errorExt);
					CuAssert(tc, "There should be no external error.", res == KSI_OK && errorExt == 0);

					res = KSI_AsyncHandle_getErrorMessage(respHandle, &msg);
					CuAssert(tc, "There should be no error message.", res == KSI_OK && msg == NULL);

					received++;

					KSI_Signature_free(signature);
				}
				break;

			case KSI_ASYNC_STATE_ERROR:
				CuFail(tc, "Requests must succeed.");
				break;

			default:
				CuFail(tc, "Unknown state for finalized request.");
				break;
		}

		KSI_AsyncHandle_free(respHandle);
	} while (onHold);
	CuAssert(tc, "Response count mismatch.", NOF_TEST_REQUESTS == received);

	KSI_LOG_debug(ctx, "%s: CLEANUP.", __FUNCTION__);

	KSI_AsyncService_free(as);

	KSI_LOG_debug(ctx, "%s: FINISH in %fs.", __FUNCTION__, difftime(time(NULL), startTime));
}

void Test_AsyncSign_loop_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_loop_getResponse(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_loop_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_loop_getResponse(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_collect_getResponse(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	time_t startTime;
	const char **p_req = NULL;
	size_t added = 0;
	size_t receivedCount = 0;
	size_t i;
	KSI_AsyncHandle **hndls = NULL;

	KSI_LOG_debug(ctx, "%s: START (%s)", __FUNCTION__, url);
	KSI_ERR_clearErrors(ctx);
	startTime = time(NULL);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_MAX_REQUEST_COUNT, (void *)(1 << 3));
	CuAssert(tc, "Unable to set maximum request count.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void*)(NOF_TEST_REQUESTS));
	CuAssert(tc, "Unable to set request cache size.", res == KSI_OK);

	hndls = KSI_calloc(NOF_TEST_REQUESTS, sizeof(KSI_AsyncHandle*));
	CuAssert(tc, "Out of memory.", hndls != NULL);

	p_req = TEST_REQUESTS;
	while (*p_req != NULL) {
		size_t pendingCount = 0;
		KSI_AsyncHandle *reqHandle = NULL;
		KSI_DataHash *hsh = NULL;
		KSI_AggregationReq *req = NULL;

		KSI_LOG_debug(ctx, "%s: REQUEST (\"%s\").", __FUNCTION__, *p_req);

		res = KSI_DataHash_create(ctx, *p_req, strlen(*p_req), KSI_HASHALG_SHA2_256, &hsh);
		CuAssert(tc, "Unable to create data hash from string.", res == KSI_OK && hsh != NULL);

		res = KSI_AggregationReq_new(ctx, &req);
		CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

		res = KSI_AggregationReq_setRequestHash(req, hsh);
		CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

		res = KSI_AsyncAggregationHandle_new(ctx, req, &reqHandle);
		CuAssert(tc, "Unable to create async request.", res == KSI_OK && reqHandle != NULL);

		res = KSI_AsyncHandle_setRequestCtx(reqHandle, (void *)*p_req, NULL);
		CuAssert(tc, "Unable to set request context.", res == KSI_OK);

		res = KSI_AsyncService_addRequest(as, reqHandle);
		CuAssert(tc, "Unable to add request.", res == KSI_OK);

		hndls[added] = reqHandle;
		p_req++;

		res = KSI_AsyncService_getPendingCount(as, &pendingCount);
		CuAssert(tc, "Unable to get pending count.", res == KSI_OK);
		CuAssert(tc, "Pending count mitmatch.", pendingCount == ++added);
	}

	do {
		size_t prevCount = receivedCount;

		KSI_LOG_debug(ctx, "%s: RUN.", __FUNCTION__);

		res = KSI_AsyncService_run(as, NULL, NULL);
		CuAssert(tc, "Failed to run async service.", res == KSI_OK);

		res = KSI_AsyncService_getReceivedCount(as, &receivedCount);
		CuAssert(tc, "Unable to get received count.", res == KSI_OK);

		if (receivedCount == prevCount) {
			/* Wait for a while to avoid busy loop. */
			KSI_LOG_debug(ctx, "%s: SLEEP.", __FUNCTION__);
			sleep_ms(50);
		}
	} while (receivedCount != added);

	for (i = 0; i < receivedCount; i++) {
		int state = KSI_ASYNC_STATE_UNDEFINED;
		KSI_AsyncHandle *handle = NULL;

		res = KSI_AsyncService_run(as, &handle, NULL);
		CuAssert(tc, "Failed to run async service.", res == KSI_OK);

		res = KSI_AsyncHandle_getState(handle, &state);
		CuAssert(tc, "Unable to get request state.", res == KSI_OK && state != KSI_ASYNC_STATE_UNDEFINED);

		switch (state) {
			case KSI_ASYNC_STATE_RESPONSE_RECEIVED: {
					KSI_AggregationResp *resp = NULL;
					char *reqCtx = NULL;

					KSI_LOG_debug(ctx, "%s: RESPONSE.", __FUNCTION__);

					res = KSI_AsyncHandle_getRequestCtx(handle, (const void**)&reqCtx);
					CuAssert(tc, "Unable to get service request context.", res == KSI_OK && reqCtx != NULL);
					CuAssert(tc, "Service request context data mismatch.", strcmp(reqCtx, TEST_REQUESTS[i]) == 0);

					res = KSI_AsyncHandle_getAggregationResp(handle, &resp);
					CuAssert(tc, "Failed to get aggregation response.", res == KSI_OK && resp != NULL);
				}
				break;

			default:
				CuFail(tc, "State should be 'received'.");
				break;
		}

		KSI_AsyncHandle_free(handle);
	}

	KSI_LOG_debug(ctx, "%s: CLEANUP.", __FUNCTION__);

	KSI_free(hndls);
	KSI_AsyncService_free(as);

	KSI_LOG_debug(ctx, "%s: FINISH in %fs.", __FUNCTION__, difftime(time(NULL), startTime));
}

void Test_AsyncSign_collect_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_collect_getResponse(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_collect_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_collect_getResponse(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_getError(CuTest* tc, const char *url, const char *user, const char *pass, int expected, long external) {
	int res;
	KSI_AsyncService *as = NULL;
	KSI_AsyncHandle *handle = NULL;
	time_t startTime;
	size_t onHold = 0;
	KSI_AggregationReq *req = NULL;
	KSI_DataHash *hsh = NULL;

	KSI_LOG_debug(ctx, "%s: START (%s)", __FUNCTION__, url);
	KSI_ERR_clearErrors(ctx);
	startTime = time(NULL);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	KSI_LOG_debug(ctx, "%s: REQUEST", __FUNCTION__);

	res = KSI_DataHash_create(ctx, TEST_REQUESTS[0], strlen(TEST_REQUESTS[0]), KSI_HASHALG_SHA2_256, &hsh);
	CuAssert(tc, "Unable to create data hash from string.", res == KSI_OK && hsh != NULL);

	res = KSI_AggregationReq_new(ctx, &req);
	CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

	res = KSI_AggregationReq_setRequestHash(req, hsh);
	CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

	res = KSI_AsyncAggregationHandle_new(ctx, req, &handle);
	CuAssert(tc, "Unable to create async request.", res == KSI_OK && handle != NULL);

	res = KSI_AsyncService_addRequest(as, handle);
	CuAssert(tc, "Unable to add request.", res == KSI_OK);

	do {
		int state = KSI_ASYNC_STATE_UNDEFINED;
		int err = KSI_UNKNOWN_ERROR;
		long ext = 0;

		KSI_LOG_debug(ctx, "%s: RUN.", __FUNCTION__);

		handle = NULL;
		res = KSI_AsyncService_run(as, &handle, &onHold);
		CuAssert(tc, "Failed to run async service.", res == KSI_OK);

		if (handle == NULL) {
			/* There is nothing has been received. */
			/* Wait for a while to avoid busy loop. */
			KSI_LOG_debug(ctx, "%s: SLEEP.", __FUNCTION__);
			sleep_ms(50);
			continue;
		}

		res = KSI_AsyncHandle_getState(handle, &state);
		CuAssert(tc, "Unable to get request state.", res == KSI_OK && state != KSI_ASYNC_STATE_UNDEFINED);
		CuAssert(tc, "Requests must fail.", state == KSI_ASYNC_STATE_ERROR);

		KSI_LOG_debug(ctx, "%s: ERROR.", __FUNCTION__);

		res = KSI_AsyncHandle_getError(handle, &err);
		CuAssert(tc, "Unable to get request error.", res == KSI_OK);
		CuAssert(tc, "Wrong error.", err == expected);
		res = KSI_AsyncHandle_getExtError(handle, &ext);
		CuAssert(tc, "Unable to get request error.", res == KSI_OK);
		CuAssert(tc, "Wrong extenral error.", ext == external);

		KSI_AsyncHandle_free(handle);
	} while (onHold);

	KSI_LOG_debug(ctx, "%s: CLEANUP.", __FUNCTION__);

	KSI_AsyncService_free(as);

	KSI_LOG_debug(ctx, "%s: FINISH in %fs.", __FUNCTION__, difftime(time(NULL), startTime));
}


void Test_AsyncSign_useExtender_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_getError(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.extender), conf.extender.user, conf.extender.pass, KSI_INVALID_FORMAT, 0);
}

void Test_AsyncSign_useExtender_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_getError(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.extender), conf.extender.user, conf.extender.pass, KSI_HTTP_ERROR, 400);
}

static int createDummyAggrAsyncRequest(KSI_AsyncHandle **ah) {
	int res;
	KSI_AsyncHandle *tmp = NULL;
	KSI_AggregationReq *req = NULL;
	KSI_DataHash *hsh = NULL;

	res = KSI_AggregationReq_new(ctx, &req);
	if (res != KSI_OK) goto cleanup;

	res = KSI_DataHash_createZero(ctx, KSI_HASHALG_SHA2_256, &hsh);
	if (res != KSI_OK) goto cleanup;

	res = KSI_AggregationReq_setRequestHash(req, hsh);
	if (res != KSI_OK) goto cleanup;
	hsh = NULL;

	res = KSI_AsyncAggregationHandle_new(ctx, req, &tmp);
	if (res != KSI_OK) goto cleanup;
	req = NULL;

	*ah = tmp;
	tmp = NULL;
cleanup:
	KSI_DataHash_free(hsh);
	KSI_AggregationReq_free(req);
	KSI_AsyncHandle_free(tmp);
	return res;
}

static void asyncSigning_fillupCache(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	KSI_AsyncHandle *hndl = NULL;
	size_t i;

	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	KSI_ERR_clearErrors(ctx);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void*)(NOF_TEST_REQUESTS));
	CuAssert(tc, "Unable to set request cache size.", res == KSI_OK);

	/* Fill up internal cache. */
	for (i = 0; i < NOF_TEST_REQUESTS; i++) {
		hndl = NULL;
		res = createDummyAggrAsyncRequest(&hndl);
		CuAssert(tc, "Unable to create dummy request.", res == KSI_OK && hndl != NULL);

		res = KSI_AsyncService_addRequest(as, hndl);
		CuAssert(tc, "Unable to add request.", res == KSI_OK);
	}
	hndl = NULL;

	/* Try to add one more request. */
	res = createDummyAggrAsyncRequest(&hndl);
	CuAssert(tc, "Unable to create dummy request.", res == KSI_OK && hndl != NULL);

	res = KSI_AsyncService_addRequest(as, hndl);
	CuAssert(tc, "Unable to add request.", res == KSI_ASYNC_REQUEST_CACHE_FULL);

	KSI_AsyncHandle_free(hndl);
	KSI_AsyncService_free(as);
}

void Test_AsyncSign_fillupCache_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_fillupCache(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_fillupCache_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_fillupCache(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_addEmptyReq(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	KSI_AsyncHandle *handle = NULL;
	KSI_AggregationReq *req = NULL;

	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	KSI_ERR_clearErrors(ctx);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	res = KSI_AggregationReq_new(ctx, &req);
	CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

	res = KSI_AsyncAggregationHandle_new(ctx, req, &handle);
	CuAssert(tc, "Unable to create async request.", res == KSI_OK && handle != NULL);

	res = KSI_AsyncService_addRequest(as, handle);
	CuAssert(tc, "Unable to add request.", res != KSI_OK && res != KSI_ASYNC_REQUEST_CACHE_FULL);

	KSI_AsyncHandle_free(handle);
	KSI_AsyncService_free(as);
}

void Test_AsyncSign_addEmptyRequest_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_addEmptyReq(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_addEmptyRequest_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_addEmptyReq(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void Test_AsyncSign_noEndpoint_addRequest(CuTest* tc) {
	int res;
	KSI_AsyncService *as = NULL;
	KSI_AsyncHandle *hndl = NULL;

	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	KSI_ERR_clearErrors(ctx);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = createDummyAggrAsyncRequest(&hndl);
	CuAssert(tc, "Unable to create dummy request.", res == KSI_OK && hndl != NULL);

	res = KSI_AsyncService_addRequest(as, hndl);
	CuAssert(tc, "Unable to add request.", res == KSI_INVALID_STATE);

	KSI_AsyncHandle_free(hndl);
	KSI_AsyncService_free(as);
}

static void asyncSigning_runEmpty(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	KSI_AsyncHandle *hndl = NULL;
	size_t onHold = 0;

	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	KSI_ERR_clearErrors(ctx);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	res = KSI_AsyncService_run(as, &hndl, &onHold);
	CuAssert(tc, "Failed to run async service.", res == KSI_OK);
	CuAssert(tc, "Waiting count mismatch.", onHold == 0);
	CuAssert(tc, "Invalid handle returned.", hndl == NULL);

	KSI_AsyncService_free(as);
}

void Test_AsyncSign_runEmpty_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_runEmpty(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_runEmpty_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_runEmpty(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_requestConfigOnly(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	KSI_AsyncHandle *handle = NULL;
	time_t startTime;
	size_t onHold = 0;
	KSI_AggregationReq *req = NULL;
	KSI_Config *cfg = NULL;
	size_t pendingCount = 0;

	KSI_LOG_debug(ctx, "%s: START (%s)", __FUNCTION__, url);
	KSI_ERR_clearErrors(ctx);
	startTime = time(NULL);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	KSI_LOG_debug(ctx, "%s: REQUEST", __FUNCTION__);

	res = KSI_AggregationReq_new(ctx, &req);
	CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

	res = KSI_Config_new(ctx, &cfg);
	CuAssert(tc, "Unable to create config object.", res == KSI_OK && cfg != NULL);

	res = KSI_AggregationReq_setConfig(req, cfg);
	CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

	res = KSI_AsyncAggregationHandle_new(ctx, req, &handle);
	CuAssert(tc, "Unable to create async request.", res == KSI_OK && handle != NULL);

	res = KSI_AsyncService_addRequest(as, handle);
	CuAssert(tc, "Unable to add request.", res == KSI_OK);

	res = KSI_AsyncService_getPendingCount(as, &pendingCount);
	CuAssert(tc, "Unable to get pending count.", res == KSI_OK);
	CuAssert(tc, "Pending count must be 1.", pendingCount == 1);

	do {
		int state = KSI_ASYNC_STATE_UNDEFINED;
		KSI_Config *respCfg = NULL;

		KSI_LOG_debug(ctx, "%s: RUN.", __FUNCTION__);

		handle = NULL;
		res = KSI_AsyncService_run(as, &handle, &onHold);
		CuAssert(tc, "Failed to run async service.", res == KSI_OK);

		if (handle == NULL) {
			/* There is nothing has been received. */
			/* Wait for a while to avoid busy loop. */
			KSI_LOG_debug(ctx, "%s: SLEEP.", __FUNCTION__);
			sleep_ms(50);
			continue;
		}

		res = KSI_AsyncHandle_getState(handle, &state);
		CuAssert(tc, "Unable to get request state.", res == KSI_OK && state != KSI_ASYNC_STATE_UNDEFINED);
		CuAssert(tc, "Invalid handle state.", state == KSI_ASYNC_STATE_PUSH_CONFIG_RECEIVED);

		KSI_LOG_debug(ctx, "%s: RESPONSE.", __FUNCTION__);

		res = KSI_AsyncHandle_getConfig(handle, &respCfg);
		CuAssert(tc, "Unable to get server config.", res == KSI_OK && respCfg != NULL);

		KSI_AsyncHandle_free(handle);
	} while (onHold);

	KSI_LOG_debug(ctx, "%s: CLEANUP.", __FUNCTION__);

	KSI_AsyncService_free(as);

	KSI_LOG_debug(ctx, "%s: FINISH in %fs.", __FUNCTION__, difftime(time(NULL), startTime));
}

void Test_AsyncSign_requestConfigOnly_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_requestConfigOnly(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_requestConfigOnly_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_requestConfigOnly(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_requestConfigWithAggrReq(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	KSI_AsyncHandle *handle = NULL;
	time_t startTime;
	size_t onHold = 0;
	KSI_AggregationReq *req = NULL;
	KSI_Config *cfg = NULL;
	KSI_DataHash *hsh = NULL;
	size_t pendingCount = 0;
	const char *p_req = TEST_REQUESTS[0];
	char confReceived = 0;
	char respReceived = 0;

	KSI_LOG_debug(ctx, "%s: START (%s)", __FUNCTION__, url);
	KSI_ERR_clearErrors(ctx);
	startTime = time(NULL);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	KSI_LOG_debug(ctx, "%s: REQUEST", __FUNCTION__);

	res = KSI_AggregationReq_new(ctx, &req);
	CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

	res = KSI_Config_new(ctx, &cfg);
	CuAssert(tc, "Unable to create config object.", res == KSI_OK && cfg != NULL);

	res = KSI_AggregationReq_setConfig(req, cfg);
	CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

	res = KSI_DataHash_create(ctx, p_req, strlen(p_req), KSI_HASHALG_SHA2_256, &hsh);
	CuAssert(tc, "Unable to create data hash from string.", res == KSI_OK && hsh != NULL);

	res = KSI_AggregationReq_setRequestHash(req, hsh);
	CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

	res = KSI_AsyncAggregationHandle_new(ctx, req, &handle);
	CuAssert(tc, "Unable to create async request.", res == KSI_OK && handle != NULL);

	res = KSI_AsyncService_addRequest(as, handle);
	CuAssert(tc, "Unable to add request.", res == KSI_OK);

	res = KSI_AsyncService_getPendingCount(as, &pendingCount);
	CuAssert(tc, "Unable to get pending count.", res == KSI_OK);
	CuAssert(tc, "Pending count must be 2.", pendingCount == 2);

	do {
		KSI_AsyncHandle *respHandle = NULL;
		int state = KSI_ASYNC_STATE_UNDEFINED;

		KSI_LOG_debug(ctx, "%s: RUN.", __FUNCTION__);

		res = KSI_AsyncService_run(as, &respHandle, &onHold);
		CuAssert(tc, "Failed to run async service.", res == KSI_OK);

		if (respHandle == NULL) {
			/* There is nothing has been received. */
			/* Wait for a while to avoid busy loop. */
			KSI_LOG_debug(ctx, "%s: SLEEP.", __FUNCTION__);
			sleep_ms(50);
			continue;
		}

		res = KSI_AsyncHandle_getState(respHandle, &state);
		CuAssert(tc, "Unable to get request state.", res == KSI_OK && state != KSI_ASYNC_STATE_UNDEFINED);

		switch (state) {
			case KSI_ASYNC_STATE_RESPONSE_RECEIVED: {
					KSI_AggregationResp *resp = NULL;

					KSI_LOG_debug(ctx, "%s: RESPONSE.", __FUNCTION__);

					res = KSI_AsyncHandle_getAggregationResp(respHandle, &resp);
					CuAssert(tc, "Failed to get aggregation response.", res == KSI_OK && resp != NULL);

					respReceived = 1;
				}
				break;

			case KSI_ASYNC_STATE_PUSH_CONFIG_RECEIVED: {
					KSI_Config *respCfg = NULL;

					KSI_LOG_debug(ctx, "%s: CONFIG RESPONSE.", __FUNCTION__);

					res = KSI_AsyncHandle_getConfig(respHandle, &respCfg);
					CuAssert(tc, "Unable to get server config.", res == KSI_OK && respCfg != NULL);

					confReceived = 1;
				}
				break;

			case KSI_ASYNC_STATE_ERROR:
				CuFail(tc, "Requests must succeed.");
				break;

			default:
				CuFail(tc, "Unknown state for finalized request.");
				break;
		}
		KSI_AsyncHandle_free(respHandle);
	} while (onHold);
	CuAssert(tc, "Configuration response should have been received.", confReceived);
	CuAssert(tc, "Aggregation response should have been received.", respReceived);

	KSI_LOG_debug(ctx, "%s: CLEANUP.", __FUNCTION__);

	KSI_AsyncService_free(as);

	KSI_LOG_debug(ctx, "%s: FINISH in %fs.", __FUNCTION__, difftime(time(NULL), startTime));
}

void Test_AsyncSign_requestConfigWithAggrReq_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_requestConfigWithAggrReq(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_requestConfigWithAggrReq_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_requestConfigWithAggrReq(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

static void asyncSigning_requestConfigAndAggrRequest_loop(CuTest* tc, const char *url, const char *user, const char *pass) {
	int res;
	KSI_AsyncService *as = NULL;
	time_t startTime;
	const char **p_req = NULL;
	size_t onHold = 0;
	size_t nofAggrResponses = 0;
	size_t nofConfResponses = 0;
	KSI_Config *cfg = NULL;
	KSI_AsyncHandle *cfgHandle = NULL;
	KSI_AggregationReq *cfgReq = NULL;

	KSI_LOG_debug(ctx, "%s: START (%s)", __FUNCTION__, url);
	KSI_ERR_clearErrors(ctx);
	startTime = time(NULL);

	res = KSI_SigningAsyncService_new(ctx, &as);
	CuAssert(tc, "Unable to create new async service object.", res == KSI_OK && as != NULL);

	res = KSI_AsyncService_setEndpoint(as, url, user, pass);
	CuAssert(tc, "Unable to configure service endpoint.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_MAX_REQUEST_COUNT, (void *)(1 << 3));
	CuAssert(tc, "Unable to set maximum request count.", res == KSI_OK);

	res = KSI_AsyncService_setOption(as, KSI_ASYNC_OPT_REQUEST_CACHE_SIZE, (void*)(NOF_TEST_REQUESTS + 1));
	CuAssert(tc, "Unable to set request cache size.", res == KSI_OK);

	KSI_LOG_debug(ctx, "%s: CONF REQUEST.", __FUNCTION__);

	res = KSI_AggregationReq_new(ctx, &cfgReq);
	CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && cfgReq != NULL);

	res = KSI_Config_new(ctx, &cfg);
	CuAssert(tc, "Unable to create config object.", res == KSI_OK && cfg != NULL);

	res = KSI_AggregationReq_setConfig(cfgReq, cfg);
	CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

	res = KSI_AsyncAggregationHandle_new(ctx, cfgReq, &cfgHandle);
	CuAssert(tc, "Unable to create async request.", res == KSI_OK && cfgHandle != NULL);

	res = KSI_AsyncService_addRequest(as, cfgHandle);
	CuAssert(tc, "Unable to add request.", res == KSI_OK);

	p_req = TEST_REQUESTS;
	do {
		KSI_AsyncHandle *respHandle = NULL;
		int state = KSI_ASYNC_STATE_UNDEFINED;

		if (*p_req != NULL) {
			size_t pendingCount = 0;
			KSI_AsyncHandle *reqHandle = NULL;
			KSI_DataHash *hsh = NULL;
			KSI_AggregationReq *req = NULL;

			KSI_LOG_debug(ctx, "%s: REQUEST (\"%s\").", __FUNCTION__, *p_req);

			res = KSI_DataHash_create(ctx, *p_req, strlen(*p_req), KSI_HASHALG_SHA2_256, &hsh);
			CuAssert(tc, "Unable to create data hash from string.", res == KSI_OK && hsh != NULL);

			res = KSI_AggregationReq_new(ctx, &req);
			CuAssert(tc, "Unable to create aggregation request.", res == KSI_OK && req != NULL);

			res = KSI_AggregationReq_setRequestHash(req, hsh);
			CuAssert(tc, "Unable to set request data hash.", res == KSI_OK);

			res = KSI_AsyncAggregationHandle_new(ctx, req, &reqHandle);
			CuAssert(tc, "Unable to create async request.", res == KSI_OK && reqHandle != NULL);

			res = KSI_AsyncService_addRequest(as, reqHandle);
			CuAssert(tc, "Unable to add request.", res == KSI_OK);
			p_req++;

			res = KSI_AsyncService_getPendingCount(as, &pendingCount);
			CuAssert(tc, "Unable to get pending count.", res == KSI_OK);
			CuAssert(tc, "Pending count must be >0.", pendingCount > 0);
		}

		KSI_LOG_debug(ctx, "%s: RUN.", __FUNCTION__);

		res = KSI_AsyncService_run(as, &respHandle, &onHold);
		CuAssert(tc, "Failed to run async service.", res == KSI_OK);

		if (respHandle == NULL) {
			if (*p_req == NULL) {
				/* There is nothing to be sent. */
				/* Wait for a while to avoid busy loop. */
				KSI_LOG_debug(ctx, "%s: SLEEP.", __FUNCTION__);
				sleep_ms(50);
			}
			continue;
		}

		res = KSI_AsyncHandle_getState(respHandle, &state);
		CuAssert(tc, "Unable to get request state.", res == KSI_OK && state != KSI_ASYNC_STATE_UNDEFINED);

		switch (state) {
			case KSI_ASYNC_STATE_RESPONSE_RECEIVED: {
					KSI_AggregationResp *resp = NULL;

					KSI_LOG_debug(ctx, "%s: RESPONSE.", __FUNCTION__);

					res = KSI_AsyncHandle_getAggregationResp(respHandle, &resp);
					CuAssert(tc, "Failed to get aggregation response.", res == KSI_OK && resp != NULL);

					nofAggrResponses++;
				}
				break;

			case KSI_ASYNC_STATE_PUSH_CONFIG_RECEIVED: {
					KSI_Config *respCfg = NULL;

					KSI_LOG_debug(ctx, "%s: CONFIG RESPONSE.", __FUNCTION__);

					res = KSI_AsyncHandle_getConfig(respHandle, &respCfg);
					CuAssert(tc, "Unable to get server config.", res == KSI_OK && respCfg != NULL);

					nofConfResponses++;
				}
				break;

			case KSI_ASYNC_STATE_ERROR:
				CuFail(tc, "Requests must succeed.");
				break;

			default:
				CuFail(tc, "Unknown state for finalized request.");
				break;
		}

		KSI_AsyncHandle_free(respHandle);
	} while (onHold);
	CuAssert(tc, "Aggregation response count mismatch.", NOF_TEST_REQUESTS == nofAggrResponses);
	CuAssert(tc, "Configuration response count mismatch.", nofConfResponses > 0);

	KSI_LOG_debug(ctx, "%s: CLEANUP.", __FUNCTION__);

	KSI_AsyncService_free(as);

	KSI_LOG_debug(ctx, "%s: FINISH in %fs.", __FUNCTION__, difftime(time(NULL), startTime));
}

void Test_AsyncSign_requestConfigAndAggrRequest_loop_tcp(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_requestConfigAndAggrRequest_loop(tc, KSITest_composeUri(TEST_SCHEME_TCP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

void Test_AsyncSign_requestConfigAndAggrRequest_loop_http(CuTest* tc) {
	KSI_LOG_debug(ctx, "%s", __FUNCTION__);
	asyncSigning_requestConfigAndAggrRequest_loop(tc, KSITest_composeUri(TEST_SCHEME_HTTP, &conf.aggregator), conf.aggregator.user, conf.aggregator.pass);
}

CuSuite* AsyncIntegrationTests_getSuite(void) {
	CuSuite* suite = CuSuiteNew();

	/* Common test cases. */
	SUITE_ADD_TEST(suite, Test_AsyncSign_noEndpoint_addRequest);

	/* TCP test cases. */
	SUITE_ADD_TEST(suite, Test_AsyncSingningService_verifyOptions_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSingningService_verifyCacheSizeOption_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_loop_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_collect_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_useExtender_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_fillupCache_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_runEmpty_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_addEmptyRequest_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_requestConfigOnly_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_requestConfigWithAggrReq_tcp);
	SUITE_ADD_TEST(suite, Test_AsyncSign_requestConfigAndAggrRequest_loop_tcp);

	/* HTTP test cases. */
	SUITE_ADD_TEST(suite, Test_AsyncSingningService_verifyOptions_http);
	SUITE_ADD_TEST(suite, Test_AsyncSingningService_verifyCacheSizeOption_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_loop_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_collect_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_useExtender_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_fillupCache_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_runEmpty_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_addEmptyRequest_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_requestConfigOnly_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_requestConfigWithAggrReq_http);
	SUITE_ADD_TEST(suite, Test_AsyncSign_requestConfigAndAggrRequest_loop_http);

	return suite;
}
