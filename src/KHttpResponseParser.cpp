#include <stdlib.h>
#include "KHttpResponseParser.h"
#include "http.h"
#include "KHttpFieldValue.h"
#include "time_utils.h"
#include "kmalloc.h"
bool KHttpResponseParser::parse_header(KHttpRequest* rq, kgl_header_type attr, const char* val, int val_len)
{
	//printf("parse header attr=[%d] val=[%s]\n",attr,val);
	const char* end = val + val_len;
	KHttpObject* obj = rq->ctx->obj;
	switch (attr) {
	case kgl_header_etag:
		KBIT_SET(obj->index.flags, OBJ_HAS_ETAG);
		return add_header(attr, val, val_len, false);
	case kgl_header_content_range:
	{		
		const char* p = (char*)memchr(val, '/', val_len);
		if (p) {
			p++;
			if (end > p) {
				rq->ctx->content_range_length = kgl_atol((u_char *)p, end - p);
				KBIT_SET(obj->index.flags, ANSW_HAS_CONTENT_RANGE);
			}
		}
		return add_header(attr, val, val_len);
	}
	case kgl_header_content_length:
		KBIT_SET(obj->index.flags, ANSW_HAS_CONTENT_LENGTH);
		if (val_len == 0) {
			obj->index.content_length = *(int64_t*)val;
		} else {
			obj->index.content_length = kgl_atol((u_char*)val, val_len);
		}
		return true;
	case kgl_header_date:
		serverDate = kgl_parse_http_time((u_char*)val, val_len);
		return add_header(attr, val, val_len);
	case kgl_header_last_modified:
		if (val_len > 0) {
			obj->data->i.last_modified = kgl_parse_http_time((u_char*)val, val_len);
		} else {
			obj->data->i.last_modified = *(time_t*)val;
			val_len = kgl_value_type(KGL_HEADER_VALUE_TIME);
		}
		if (obj->data->i.last_modified > 0) {
			obj->index.flags |= ANSW_LAST_MODIFIED;
		}
		return add_header(attr, val, val_len);
	case kgl_header_set_cookie:
		if (!KBIT_TEST(obj->index.flags, OBJ_IS_STATIC2)) {
			obj->index.flags |= ANSW_NO_CACHE;
		}
		return add_header(attr, val, val_len);
	case kgl_header_pragma:
	{
		KHttpFieldValue field(val, val + val_len);
		do {
			if (field.is(_KS("no-cache"))) {
				obj->index.flags |= ANSW_NO_CACHE;
				break;
			}
		} while (field.next());
		return add_header(attr, val, val_len);
	}
	case kgl_header_cache_control:
	{
		KHttpFieldValue field(val, val + val_len);
		do {
#ifdef ENABLE_STATIC_ENGINE
			if (!KBIT_TEST(obj->index.flags, OBJ_IS_STATIC2)) {
#endif
				if (field.is(_KS("no-store"))) {
					obj->index.flags |= ANSW_NO_CACHE;
				} else if (field.is(_KS("no-cache"))) {
					obj->index.flags |= ANSW_NO_CACHE;
				} else if (field.is(_KS("private"))) {
					obj->index.flags |= ANSW_NO_CACHE;
				}
#ifdef ENABLE_STATIC_ENGINE
			}
#endif
#ifdef ENABLE_FORCE_CACHE
			if (field.is(_KS("static"))) {
				//通过http header强制缓存
				obj->force_cache();
			} else
#endif
				if (field.is(_KS("public"))) {
					KBIT_CLR(obj->index.flags, ANSW_NO_CACHE);
				} else if (field.is(_KS("max-age="), (int*)&obj->data->i.max_age)) {
					obj->index.flags |= ANSW_HAS_MAX_AGE;
				} else if (field.is(_KS("s-maxage="), (int*)&obj->data->i.max_age)) {
					obj->index.flags |= ANSW_HAS_MAX_AGE;
				} else if (field.is(_KS("must-revalidate"))) {
					obj->index.flags |= OBJ_MUST_REVALIDATE;
				}
		} while (field.next());
#ifdef ENABLE_FORCE_CACHE
		if (KBIT_TEST(obj->index.flags, OBJ_IS_STATIC2)) {
			return true;
		}
#endif
		return add_header(attr, val, val_len);
	}
	case kgl_header_vary:
		if (obj->AddVary(rq, val, val_len)) {
			return true;
		}
		return add_header(attr, val, val_len);
	case kgl_header_age:
		age = kgl_atoi((u_char*)val, val_len);
		return true;
	case kgl_header_content_encoding:
		if (kgl_mem_case_same(val, val_len, _KS("none"))) {
			return true;
		}
		if (kgl_mem_case_same(val, val_len, _KS("gzip"))) {
			obj->uk.url->set_content_encoding(KGL_ENCODING_GZIP);
		} else	if (kgl_mem_case_same(val, val_len, _KS("deflate"))) {
			obj->uk.url->set_content_encoding(KGL_ENCODING_DEFLATE);
		} else if (kgl_mem_case_same(val, val_len, _KS("compress"))) {
			obj->uk.url->set_content_encoding(KGL_ENCODING_COMPRESS);
		} else if (kgl_mem_case_same(val, val_len, _KS("br"))) {
			obj->uk.url->set_content_encoding(KGL_ENCODING_BR);
		} else if (kgl_mem_case_same(val, val_len, _KS("identity"))) {
			obj->uk.url->accept_encoding = (u_char)~0;
		} else if (val_len > 0) {
			//不明content-encoding不能缓存
			KBIT_SET(obj->index.flags, FLAG_DEAD);
			obj->uk.url->set_content_encoding(KGL_ENCODING_UNKNOW);
		}
		return add_header(attr, val, val_len);
	case kgl_header_expires:
		if (!KBIT_TEST(obj->index.flags, ANSW_HAS_EXPIRES)) {
			KBIT_SET(obj->index.flags, ANSW_HAS_EXPIRES);
			expireDate = kgl_parse_http_time((u_char*)val, val_len);		
		}
		return add_header(attr, val, val_len);
	default:
		return add_header(attr, val, val_len);
	}
	return false;
}
bool KHttpResponseParser::parse_unknow_header(KHttpRequest* rq, const char* attr, int attr_len, const char* val, int val_len, bool request_line)
{
	//printf("parse unknow header attr=[%s] val=[%s]\n",attr,val);
	KHttpObject* obj = rq->ctx->obj;
	if (*attr == 'x' || *attr == 'X') {
		if (!KBIT_TEST(rq->filter_flags, RF_NO_X_SENDFILE) &&
			(kgl_mem_case_same(attr, attr_len, _KS("X-Accel-Redirect")) || kgl_mem_case_same(attr, attr_len, _KS("X-Proxy-Redirect")))) {
			KBIT_SET(obj->index.flags, ANSW_XSENDFILE);
			return add_header(attr, attr_len, val, val_len, false);
		}
		if (kgl_mem_case_same(attr, attr_len, _KS("X-Gzip"))) {
			obj->need_compress = 1;
			return true;
		}
	}
	return add_header(attr, attr_len, val, val_len);
}
void KHttpResponseParser::end_parse(KHttpRequest* rq)
{
	/*
 * see rfc2616
 * 没有 Last-Modified 我们不缓存.
 * 但如果有 expires or max-age  除外
 */
	if (!KBIT_TEST(rq->ctx->obj->index.flags, ANSW_LAST_MODIFIED | OBJ_HAS_ETAG)) {
		if (!KBIT_TEST(rq->ctx->obj->index.flags, ANSW_HAS_MAX_AGE | ANSW_HAS_EXPIRES)) {
			KBIT_SET(rq->ctx->obj->index.flags, ANSW_NO_CACHE);
		}
	}
	if (!KBIT_TEST(rq->ctx->obj->index.flags, ANSW_NO_CACHE)) {
		if (serverDate == 0) {
			serverDate = kgl_current_sec;
		}
		time_t responseTime = kgl_current_sec;
		/*
		 * the age calculation algorithm see rfc2616 sec 13
		 */
		unsigned apparent_age = 0;
		if (responseTime > serverDate) {
			apparent_age = (unsigned)(responseTime - serverDate);
		}
		unsigned corrected_received_age = MAX(apparent_age, age);

		unsigned response_delay = (unsigned)(responseTime - rq->sink->data.begin_time_msec / 1000);
		unsigned corrected_initial_age = corrected_received_age + response_delay;
		unsigned resident_time = (unsigned)(kgl_current_sec - responseTime);
		age = corrected_initial_age + resident_time;
		if (!KBIT_TEST(rq->ctx->obj->index.flags, ANSW_HAS_MAX_AGE)
			&& KBIT_TEST(rq->ctx->obj->index.flags, ANSW_HAS_EXPIRES)) {
			rq->ctx->obj->data->i.max_age = (unsigned)(expireDate - serverDate) - age;
		}
	}
	commit_headers(rq);
}
void KHttpResponseParser::commit_headers(KHttpRequest* rq)
{
	if (last) {
		last->next = rq->ctx->obj->data->headers;
		rq->ctx->obj->data->headers = steal_header();
	}
}

kgl_header_type kgl_parse_response_header(const char* attr, hlen_t attr_len)
{
	if (kgl_mem_case_same(attr, attr_len, _KS("Etag"))) {
		return kgl_header_etag;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Content-Range"))) {
		return kgl_header_content_range;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Content-length"))) {
		return kgl_header_content_length;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Content-Type"))) {
		return kgl_header_content_type;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Date"))) {
		return kgl_header_date;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Last-Modified"))) {
		return kgl_header_last_modified;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Set-Cookie"))) {
		return kgl_header_set_cookie;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Pragma"))) {
		return kgl_header_pragma;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Cache-Control"))) {
		return kgl_header_cache_control;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Vary"))) {
		return kgl_header_vary;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Age"))) {
		return kgl_header_age;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Alt-Svc"))) {
		return kgl_header_alt_svc;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Keep-Alive"))) {
		return kgl_header_keep_alive;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Connection"))) {
		return kgl_header_connection;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Transfer-Encoding"))) {
		return kgl_header_transfer_encoding;
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Content-Encoding"))) {
		return kgl_header_content_encoding;
	
	}
	if (kgl_mem_case_same(attr, attr_len, _KS("Expires"))) {
		return kgl_header_expires;
	}
	return kgl_header_unknow;
}
