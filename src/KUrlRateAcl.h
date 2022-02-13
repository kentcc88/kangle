#ifndef KURLRATEACL_H
#define KURLRATEACL_H
#include "KAcl.h"
#include "KTargetRate.h"
//{{ent
#ifdef ENABLE_BLACK_LIST
class KUrlRateAcl : public KAcl
{
public:
	KUrlRateAcl()
	{
		lastFlushTime = 0;
		request = 0;
		second = 0;
	}
	~KUrlRateAcl()
	{
	}
	bool supportRuntime()
	{
		return true;
	}
	KAcl *newInstance() {
		return new KUrlRateAcl();
	}
	const char *getName() {
		return "url_rate";
	}	
	std::string getDisplay() {
		std::stringstream s;
		s << "&gt;" << request << "/" << second << "s ";
		s << rate.getCount();
		return s.str();
	}
	void editHtml(std::map<std::string, std::string> &attribute,bool html) {
		request = atoi(attribute["request"].c_str());
		second = atoi(attribute["second"].c_str());
	}
	void buildXML(std::stringstream &s) {
		s << "request='" << request << "' second='" << second << "'>";
	}
	std::string getHtml(KModel *model) {
		std::stringstream s;
		KUrlRateAcl *m = (KUrlRateAcl *)model;
		s << "rate&gt;request:<input name='request' size=4 value='";
		if (m) {
			s << m->request;
		}
		s << "'>";
		s << "second:<input name='second' size=4 value='";
		if (m) {
			s << m->second;
		}		
		s << "'>";
		return s.str();
	}
	bool match(KHttpRequest *rq, KHttpObject *obj) {
		if (kgl_current_sec - lastFlushTime > 5) {
			rate.flush(kgl_current_sec,second);
			lastFlushTime = kgl_current_sec;
		}
		KStringBuf target(2048);
		target << (int)rq->url->flags << "_" << rq->url->host << ":" << rq->url->port << rq->url->path;
		char *ip_url = target.getString();
		int r = request;
		int s;
		bool result = rate.getRate(ip_url,r,s,true);
		if (!result) {
			return false;
		}
		if (second>s) {
			return true;
		}
		return false;
	}
private:
	int request;
	int second;
	KTargetRate rate;
	time_t lastFlushTime;
};
#endif
//}}
#endif
