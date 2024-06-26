#ifndef KAPACHEVIRTUALHOST_H
#define KAPACHEVIRTUALHOST_H
#include "KHtModule.h"
struct KApacheVirtualHostItem {
	int port;
	std::string serverName;
	std::list<std::string> hosts;
	std::string documentRoot;
	std::string log;
	std::string certificate;
	std::string certificate_key;
	std::vector<std::string> index;
	void buildIndex(KStringBuf &s)
	{
		for(size_t i=0;i<index.size();i++){
			s << "<index id='100' file='" << index[i] << "'/>\n";
		}
	}
};
class KApacheVirtualHost : public KHtModule
{
public:
	KApacheVirtualHost()
	{
		vitem = NULL;
	}
	~KApacheVirtualHost()
	{
		if(vitem){
			delete vitem;
		}
		std::list<KApacheVirtualHostItem *>::iterator it;
		for(it=vitems.begin();it!=vitems.end();it++){
			delete (*it);
		}
	}
	bool startContext(KApacheConfig *htaccess,const char *cmd,std::map<char *,char *,lessp_icase> &attribute) override;
	bool endContext(KApacheConfig *htaccess,const char *cmd) override;
	bool process(KApacheConfig *htaccess,const char *cmd,std::vector<char *> &item) override;
	bool getXml(KStringBuf&s) override;
	
private:
	KApacheVirtualHostItem *vitem;
	std::list<KApacheVirtualHostItem *> vitems;
	KApacheVirtualHostItem global;
	std::string user;
	std::string group;
};
#endif
