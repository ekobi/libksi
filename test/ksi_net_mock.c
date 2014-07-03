#include <limits.h>
#include <string.h>

#include "../src/ksi/internal.h"

unsigned char *KSI_NET_MOCK_request = NULL;
unsigned KSI_NET_MOCK_request_len;
unsigned char *KSI_NET_MOCK_response = NULL;
unsigned KSI_NET_MOCK_response_len;

static int mockPublicationsFileReceive(KSI_RequestHandle *handle) {
	int res = KSI_UNKNOWN_ERROR;
	FILE *f = NULL;
	unsigned char *raw = NULL;
	unsigned len;
	long int raw_size = 0;

	if (handle == NULL) goto cleanup;

	KSI_LOG_debug(KSI_RequestHandle_getCtx(handle), "Connecting to MOCK publications file service");

	f = fopen("test/resource/tlv/publications.tlv", "rb");
	if (f == NULL) {
		res = KSI_IO_ERROR;
		goto cleanup;
	}

	res = fseek(f, 0, SEEK_END);
	if (res != 0) {
		res = KSI_IO_ERROR;
		goto cleanup;
	}

	raw_size = ftell(f);
	if (raw_size < 0) {
		res = KSI_IO_ERROR;
		goto cleanup;
	}

	if (raw_size > INT_MAX) {
		res = KSI_INVALID_FORMAT;
		goto cleanup;
	}

	res = fseek(f, 0, SEEK_SET);
	if (res != 0) {
		res = KSI_IO_ERROR;
		goto cleanup;
	}

	raw = KSI_calloc((unsigned)raw_size, 1);
	if (raw == NULL) {
		res = KSI_OUT_OF_MEMORY;
		goto cleanup;
	}

	len = (unsigned)fread(raw, 1, (unsigned)raw_size, f);
	if (len != raw_size) {
		res = KSI_IO_ERROR;
		goto cleanup;
	}


	res = KSI_RequestHandle_setResponse(handle, raw, len);
	if (res != KSI_OK) goto cleanup;

cleanup:
		KSI_free(raw);
		if (f != NULL) fclose(f);

		return res;
}



static int mockReceive(KSI_RequestHandle *handle) {
	int res = KSI_UNKNOWN_ERROR;

	if (handle == NULL) goto cleanup;

	KSI_LOG_debug(KSI_RequestHandle_getCtx(handle), "Connecting to MOCK service");

	res = KSI_RequestHandle_setResponse(handle, KSI_NET_MOCK_response, KSI_NET_MOCK_response_len);
	if (res != KSI_OK) goto cleanup;

cleanup:

		return res;
}

static int mockSend(KSI_RequestHandle *handle) {
	int res = KSI_UNKNOWN_ERROR;
	const unsigned char *req = NULL;
	unsigned req_len;

	KSI_LOG_debug(KSI_RequestHandle_getCtx(handle), "Initiate MOCK request.");

	res = KSI_RequestHandle_setReadResponseFn(handle, mockReceive);
	if (res != KSI_OK) goto cleanup;

	res = KSI_RequestHandle_getRequest(handle, &req, &req_len);
	if (res != KSI_OK) goto cleanup;

	if (KSI_NET_MOCK_request != NULL) KSI_free((unsigned char *)KSI_NET_MOCK_request);

	KSI_NET_MOCK_request = KSI_calloc(req_len, 1);
	memcpy((unsigned char *)KSI_NET_MOCK_request, req, req_len);

	KSI_NET_MOCK_request_len = req_len;
	res = KSI_OK;
cleanup:

	return res;
}

static int mockSendSignRequest(KSI_NetworkClient *netProvider, KSI_RequestHandle *handle) {
	return mockSend(handle);
}

static int mockSendExtendRequest(KSI_NetworkClient *netProvider, KSI_RequestHandle *handle) {
	return mockSend(handle);
}

static int mockSendPublicationsFileRequest(KSI_NetworkClient *netProvider, KSI_RequestHandle *handle) {
	int res = KSI_UNKNOWN_ERROR;
	const unsigned char *req = NULL;
	unsigned req_len;

	KSI_LOG_debug(KSI_RequestHandle_getCtx(handle), "Initiate MOCK request.");

	res = KSI_RequestHandle_setReadResponseFn(handle, mockPublicationsFileReceive);
	if (res != KSI_OK) goto cleanup;

	res = KSI_RequestHandle_getRequest(handle, &req, &req_len);
	if (res != KSI_OK) goto cleanup;

	if (KSI_NET_MOCK_request != NULL) KSI_free((unsigned char *)KSI_NET_MOCK_request);

	KSI_NET_MOCK_request = KSI_calloc(req_len, 1);
	memcpy((unsigned char *)KSI_NET_MOCK_request, req, req_len);

	KSI_NET_MOCK_request_len = req_len;
	res = KSI_OK;
cleanup:

	return res;
}

int KSI_NET_MOCK_new(KSI_CTX *ctx, KSI_NetworkClient **provider) {
	KSI_ERR err;
	int res;
	KSI_NetworkClient *pr = NULL;

	KSI_PRE(&err, ctx != NULL) goto cleanup;
	KSI_BEGIN(ctx, &err);

	KSI_NET_MOCK_request_len = 0;
	KSI_NET_MOCK_response_len = 0;

	res = KSI_NetworkClient_new(ctx, &pr);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_NetworkClient_setSendSignRequestFn(pr, mockSendSignRequest);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_NetworkClient_setSendExtendRequestFn(pr, mockSendExtendRequest);
	KSI_CATCH(&err, res) goto cleanup;

	res = KSI_NetworkClient_setSendPublicationRequestFn(pr, mockSendPublicationsFileRequest);
	KSI_CATCH(&err, res) goto cleanup;

	*provider = pr;
	pr = NULL;

	KSI_SUCCESS(&err);

cleanup:

	KSI_free(pr);

	return KSI_RETURN(&err);
}
