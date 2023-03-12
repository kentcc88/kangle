/*
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
#ifndef KSPEEDLIMITMARK_H_
#define KSPEEDLIMITMARK_H_
#include<string>
#include<map>
#include "KMark.h"
#include "do_config.h"
#include "KSpeedLimit.h"

class KSpeedLimitMark : public KMark {
public:
	KSpeedLimitMark() {
		speed_limit=0;
	}	
	virtual ~KSpeedLimitMark() {
	}
	bool mark(KHttpRequest *rq, KHttpObject *obj, KFetchObject** fo) override {
		KSpeedLimit *sl = new KSpeedLimit;
		sl->setSpeedLimit(speed_limit);
		rq->pushSpeedLimit(sl);
		return true;
	}
	void get_display(KWStream &s) override {
		s << speed_limit;
	}
	void parse_config(const khttpd::KXmlNodeBody* xml) override {
		auto attribute = xml->attr();
		speed_limit=(int)get_size(attribute["limit"].c_str());
	}
	void get_html(KModel *model,KWStream &s) override {
		KSpeedLimitMark *mark = (KSpeedLimitMark *)model;
		s << "limit:<input name='limit' size=10 value='";
		if (mark) {
			s << mark->speed_limit;
		}
		s << "'>";
	}
	KMark * new_instance()override {
		return new KSpeedLimitMark();
	}
	const char *getName()override {
		return "speed_limit";
	}
private:
	int speed_limit;
};
#endif /*KSPEEDLIMITMARK_H_*/
