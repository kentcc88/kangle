/*
 * KFetchObject.h
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

#ifndef KFETCHOBJECT_H_
#define KFETCHOBJECT_H_
#include <time.h>
#include "KBuffer.h"
#include "KPathRedirect.h"
#include "kselector.h"
#include "ksapi.h"
#include "KPushGate.h"
#define READ_SWITCH_FUNCTION	-3
#define READ_PROTOCOL_ERROR		-2
#define SEND_HEAD_FAILED		0
#define SEND_HEAD_SUCCESS		1
#define SEND_HEAD_PULL_MODEL	2
 //#define SEND_HEAD_ASYNC_MODEL   3
#define SEND_HEAD_SYNC_MODEL    4
#define SEND_HEAD_ERROR         5
#ifdef ENABLE_REQUEST_QUEUE
class KRequestQueue;
#endif
class KHttpObject;
class KHttpRequest;
class KContext;
class KUpstream;
/**
数据源类(静态数据，动态数据，除缓存的数据外，所有请求的数据均来源于数据源)
所有数据源的基类
*/
class KFetchObject
{
public:
	KFetchObject() {
	}
	virtual ~KFetchObject();
	virtual void on_readhup(KHttpRequest* rq) {

	}
	virtual KGL_RESULT Open(KHttpRequest* rq, kgl_input_stream* in, kgl_output_stream* out) = 0;

	virtual bool NeedTempFile(bool upload, KHttpRequest* rq);
	virtual bool is_filter() {
		return false;
	}
	virtual bool before_cache() {
		return false;
	}
#ifdef ENABLE_REQUEST_QUEUE
	//此数据源是否需要队列功能。对于本地数据不用该功能。
	//对于上游数据和动态的则返回true
	virtual bool needQueue(KHttpRequest* rq) {
		return false;
	}
#endif
	static KGL_RESULT PushBody(KHttpRequest* rq, kgl_response_body* out, const char* buf, int len);
	KFetchObject* next = nullptr;
};
class KRedirectSource : public KFetchObject
{
public:
	KRedirectSource() {
		brd = nullptr;
	}
	virtual ~KRedirectSource() {
		if (brd) {
			brd->release();
		}
	}
	void bind_base_redirect(KBaseRedirect* brd) {
		kassert(this->brd == NULL);
		this->brd = brd;
	}
protected:
	KBaseRedirect* brd;
};
class KDeniedSource : public KFetchObject
{
public:
	KDeniedSource(uint16_t status_code, const char* reason, size_t reason_len) {
		this->status_code = status_code;
		this->reason = reason;
		this->reason_len = int(reason_len);
	}
	KDeniedSource(const char* reason, size_t reason_len) : KDeniedSource(STATUS_FORBIDEN, reason, reason_len) {

	}
	bool before_cache() override {
		return true;
	}
	KGL_RESULT Open(KHttpRequest* rq, kgl_input_stream* in, kgl_output_stream* out) override {
		return out->f->error(out->ctx, status_code, reason, reason_len);
	}
private:
	const char* reason;
	int reason_len;
	uint16_t status_code;
};
#endif /* KFETCHOBJECT_H_ */
