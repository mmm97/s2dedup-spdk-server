/*-
 *   BSD LICENSE
 *
 *   Copyright (c) Intel Corporation.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "vbdev_persistent_dedupas.h"
#include "spdk/rpc.h"
#include "spdk/util.h"
#include "spdk/string.h"
#include "spdk_internal/log.h"

/* Structure to hold the parameters for this RPC method. */
struct rpc_bdev_dedupas_create {
	char *base_bdev_name;
	char *name;
	char *blocklen;
};

/* Free the allocated memory resource after the RPC handling. */
static void
free_rpc_bdev_dedupas_create(struct rpc_bdev_dedupas_create *r)
{
	free(r->base_bdev_name);
	free(r->name);
}

/* Structure to decode the input parameters for this RPC method. */
static const struct spdk_json_object_decoder rpc_bdev_dedupas_create_decoders[] = {
	{"base_bdev_name", offsetof(struct rpc_bdev_dedupas_create, base_bdev_name), spdk_json_decode_string},
	{"name", offsetof(struct rpc_bdev_dedupas_create, name), spdk_json_decode_string},
	{"blocklen", offsetof(struct rpc_bdev_dedupas_create, blocklen), spdk_json_decode_string},
};

/* Decode the parameters for this RPC method and properly construct the dedupas
 * device. Error status returned in the failed cases.
 */
static void
spdk_rpc_bdev_persistent_dedupas_create(struct spdk_jsonrpc_request *request,
			      const struct spdk_json_val *params)
{
	struct rpc_bdev_dedupas_create req = {NULL};
	struct spdk_json_write_ctx *w;
	int rc;

	if (spdk_json_decode_object(params, rpc_bdev_dedupas_create_decoders,
				    SPDK_COUNTOF(rpc_bdev_dedupas_create_decoders),
				    &req)) {
		SPDK_DEBUGLOG(SPDK_LOG_VBDEV_DEDUPAS, "spdk_json_decode_object failed\n");
		spdk_jsonrpc_send_error_response(request, SPDK_JSONRPC_ERROR_INTERNAL_ERROR,
						 "spdk_json_decode_object failed");
		goto cleanup;
	}

	rc = bdev_dedupas_create_disk(req.base_bdev_name, req.name, atoi(req.blocklen));
	if (rc != 0) {
		spdk_jsonrpc_send_error_response(request, rc, spdk_strerror(-rc));
		goto cleanup;
	}

	w = spdk_jsonrpc_begin_result(request);
	spdk_json_write_string(w, req.name);
	spdk_jsonrpc_end_result(request, w);

cleanup:
	free_rpc_bdev_dedupas_create(&req);
}                  
SPDK_RPC_REGISTER("bdev_persistent_dedupas_create", spdk_rpc_bdev_persistent_dedupas_create, SPDK_RPC_RUNTIME)
SPDK_RPC_REGISTER_ALIAS_DEPRECATED(bdev_persistent_dedupas_create, construct_persistent_dedupas_bdev)

struct rpc_bdev_dedupas_delete {
	char *name;
};

static void
free_rpc_bdev_dedupas_delete(struct rpc_bdev_dedupas_delete *req)
{
	free(req->name);
}

static const struct spdk_json_object_decoder rpc_bdev_dedupas_delete_decoders[] = {
	{"name", offsetof(struct rpc_bdev_dedupas_delete, name), spdk_json_decode_string},
};

static void
_spdk_rpc_bdev_dedupas_delete_cb(void *cb_arg, int bdeverrno)
{
	struct spdk_jsonrpc_request *request = cb_arg;
	struct spdk_json_write_ctx *w;

	w = spdk_jsonrpc_begin_result(request);
	spdk_json_write_bool(w, bdeverrno == 0);
	spdk_jsonrpc_end_result(request, w);
}

static void
spdk_rpc_bdev_persistent_dedupas_delete(struct spdk_jsonrpc_request *request,
			      const struct spdk_json_val *params)
{
	struct rpc_bdev_dedupas_delete req = {NULL};
	struct spdk_bdev *bdev;

	if (spdk_json_decode_object(params, rpc_bdev_dedupas_delete_decoders,
				    SPDK_COUNTOF(rpc_bdev_dedupas_delete_decoders),
				    &req)) {
		spdk_jsonrpc_send_error_response(request, SPDK_JSONRPC_ERROR_INTERNAL_ERROR,
						 "spdk_json_decode_object failed");
		goto cleanup;
	}

	bdev = spdk_bdev_get_by_name(req.name);
	if (bdev == NULL) {
		spdk_jsonrpc_send_error_response(request, -ENODEV, spdk_strerror(ENODEV));
		goto cleanup;
	}

	bdev_dedupas_delete_disk(bdev, _spdk_rpc_bdev_dedupas_delete_cb, request);

cleanup:
	free_rpc_bdev_dedupas_delete(&req);
}
SPDK_RPC_REGISTER("bdev_persistent_dedupas_delete", spdk_rpc_bdev_persistent_dedupas_delete, SPDK_RPC_RUNTIME)
SPDK_RPC_REGISTER_ALIAS_DEPRECATED(bdev_persistent_dedupas_delete, delete_persistent_dedupas_bdev)