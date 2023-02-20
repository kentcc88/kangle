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
#ifndef KTRYFILEACL_H_
#define KTRYFILEACL_H_
#include "KXml.h"
class KTryFileAcl: public KAcl {
public:
	KTryFileAcl() {

	}
	virtual ~KTryFileAcl() {
	}
	KAcl *new_instance() override {
		return new KTryFileAcl();
	}
	const char *getName() override {
		return "try_file";
	}
	bool match(KHttpRequest *rq, KHttpObject *obj) override {
		auto svh = rq->get_virtual_host();
		if (svh==NULL) {
			return false;
		}
		KFileName file;
		bool exsit = false;
		KVirtualHost *vh = svh->vh;
		if (!vh->alias(rq->ctx.internal,rq->sink->data.url->path,&file,exsit,rq->getFollowLink())) {
			exsit = file.setName(svh->doc_root, rq->sink->data.url->path, rq->getFollowLink());
		}
		if (file.isDirectory()) {			
			KFileName *defaultFile = NULL;
			exsit = vh->getIndexFile(rq,&file,&defaultFile,NULL);
			if (defaultFile) {
				delete defaultFile;
			}
		}
		return exsit;
	}
	std::string getHtml(KModel *model) override {
		return "";
	}	
	std::string getDisplay() override {
		return "";
	}
	void editHtml(std::map<std::string, std::string> &attibute,bool html) override {
		
	}
	void buildXML(std::stringstream &s) override {
		s << " >";
	}
};

#endif
