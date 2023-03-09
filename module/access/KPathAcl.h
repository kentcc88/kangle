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
#ifndef KPATHACL_H_
#define KPATHACL_H_

#include "KAcl.h"
#include "KReg.h"
#include "KXml.h"
#include "KStringBuf.h"
class KPathAcl: public KAcl {
public:
	KPathAcl() {
		wide = false;
		raw = false;
	}
	virtual ~KPathAcl() {
	}
	std::string getHtml(KModel *model) override {
		std::stringstream s;
		s << "<input name=path value='";
		KPathAcl *urlAcl = (KPathAcl *) (model);
		if (urlAcl) {
			s << urlAcl->getPath();
		}
		s << "'>";
		s << "<input name='raw' value='1' type='checkbox' ";
		if (urlAcl && urlAcl->raw) {
			s << "checked";
		}
		s << ">raw";
		return s.str();
	}
	KAcl *new_instance() override {
		return new KPathAcl();
	}
	const char *getName() override {
		return "path";
	}
	bool match(KHttpRequest *rq, KHttpObject *obj) override {
		KUrl *url;
		if (raw) {
			url = rq->sink->data.raw_url;
		} else {
			url = rq->sink->data.url;
		}
		if(wide){
			return filencmp(url->path,path.c_str(),path.size()) == 0;
		}
		return filecmp(url->path, path.c_str()) == 0;
	}
	std::string getDisplay() override {
		std::stringstream s;
		if (raw) {
			s << "raw:";
		}
		s << getPath();
		return s.str();
	}

	bool setPath(const char *value) {
		wide = false;
		path = "";
		if (value[0] != '/') {
			path = "/";
		}
		path += value;
		char *t = strdup(path.c_str());
		char *p = strchr(t, '?');
		if (p) {
			*p = 0;
		}
		if(t[strlen(t)-1] == '*'){
			wide = true;
			t[strlen(t)-1] = '\0';	
		}
		path = t;
		xfree(t);
		return true;
	}
	void parse_config(const khttpd::KXmlNodeBody* xml) override {
		auto attribute = xml->attr();
		if (attribute["path"].size() > 0) {
			setPath(attribute["path"].c_str());
		} else {
			setPath(xml->get_text());
		}
		if (attribute["raw"]=="1") {
			raw = true;
		} else {
			raw = false;
		}

	}
private:
	std::string getPath()
	{
		std::stringstream s;
		s << path << (wide?"*":"");
		return s.str();
	}
	std::string path;
	bool wide;
	bool raw;
};

#endif /*KURLACL_H_*/
