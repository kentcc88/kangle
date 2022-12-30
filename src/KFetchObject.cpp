/*
 * KFetchObject.cpp
 *
 *  Created on: 2010-4-19
 *      Author: keengo
 * Copyright (c) 2010, NanChang BangTeng Inc
 * All Rights Reserved.
 *
 * You may use the Software for free for non-commercial use
 * under the License Restrictions.
 *
 * You may modify the source code(if being provieded) or interface
 * of the Software under the License Restrictions.
 *
 * You may use the Software for commercial use after purchasing the
 * commercial license.Moreover, according to the license you purchased
 * you may get specified term, manner and content of technical
 * support from NanChang BangTeng Inc
 *
 * See COPYING file for detail.
 */
#include "KFetchObject.h"
#include "kmalloc.h"
#include "KAsyncFetchObject.h"
#include "KHttpTransfer.h"
#include "HttpFiber.h"

KFetchObject::~KFetchObject()
{
	if (brd) {
		brd->release();
	}
}

bool KFetchObject::NeedTempFile(bool upload, KHttpRequest *rq)
{
	if (!upload) {
		return true;
	}
	return rq->sink->data.content_length == -1;
}
KGL_RESULT KFetchObject::PushBody(KHttpRequest *rq, kgl_output_stream *out, const char *buf, int len)
{
	if (!KBIT_TEST(rq->sink->data.flags, RQ_CONNECTION_UPGRADE) && rq->ctx->know_length) {
		len = (int)KGL_MIN(rq->ctx->left_read, (INT64)len);
		rq->ctx->left_read -= len;
	}
	return out->f->write_body(out, rq, buf, len);
}
#if 0
uint32_t KRefFetchObject::Check(KHttpRequest *rq)
{
	assert(rq->GetNextFetchObject(this) == NULL);
	klist_remove(&queue);
	if (this->fo) {
		rq->AppendFetchObject(this->fo);
		this->fo = NULL;
	}
	delete this;
	return KF_STATUS_REQ_FALSE;
	/*
	KGL_RESULT ret = fiber_http_start(rq);
	delete this;
	return ret;
	*/
}

#endif
