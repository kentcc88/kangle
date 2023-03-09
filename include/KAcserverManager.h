#ifndef KACSERVERMANAGER_H_
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
#define KACSERVERMANAGER_H_
#include <vector>
#include "KApiRedirect.h"
#include "KSingleAcserver.h"
#include "KMultiAcserver.h"
#include "KCmdPoolableRedirect.h"
#include "KRWLock.h"
#include "KExtendProgram.h"
#include "KConfigTree.h"
#include <string>
class KAcserverManager
{
public:
	KAcserverManager();
	virtual ~KAcserverManager();
	void on_server_event(kconfig::KConfigTree* tree, const khttpd::KXmlNode* xml, kconfig::KConfigEventType ev);
	void on_cmd_event(kconfig::KConfigTree* tree, const khttpd::KXmlNode* xml, kconfig::KConfigEventType ev);
	void on_api_event(kconfig::KConfigTree* tree, const khttpd::KXmlNode* xml, kconfig::KConfigEventType ev);
	std::string acserverList(const std::string& name = "");
	std::string apiList(const std::string& name = "");
#ifdef ENABLE_VH_RUN_AS	
	std::string cmdList(const std::string& name = "");
#ifdef ENABLE_ADPP
	/*
	 * flush the cmd extend process cpu usage.
	 */
	void flushCpuUsage(ULONG64 cpuTime);
#endif
	void shutdown();
	void refreshCmd(time_t nowTime);
	void getProcessInfo(std::stringstream& s);
	void killCmdProcess(USER_T user);
	void killAllProcess(KVirtualHost* vh);
	/* 全部准备好了，可以加载所有的api了。*/
	void loadAllApi();
#endif
	void unloadAllApi();
	bool remove_server(const std::string &name, std::string& err_msg);
#ifdef ENABLE_MULTI_SERVER
	std::string macserverList(const std::string& name="");
	std::string macserver_node_form(const std::string&, std::string action, unsigned nodeIndex);
	bool macserver_node(KXmlAttribute& attribute, std::string& errMsg);
	KMultiAcserver* refsMultiAcserver(const std::string& name)
	{
		lock.RLock();
		KMultiAcserver* as = getMultiAcserver(name);
		if (as) {
			as->add_ref();
		}
		lock.RUnlock();
		return as;
	}
	bool addMultiAcserver(KMultiAcserver* as)
	{
		lock.WLock();
		std::map<std::string, KMultiAcserver*>::iterator it = mservers.find(as->name);
		if (it != mservers.end()) {
			lock.WUnlock();
			return false;
		}
		mservers.insert(std::pair<std::string, KMultiAcserver*>(as->name, as));
		lock.WUnlock();
		return true;
	}
#endif
	std::vector<std::string> getAcserverNames(bool onlyHttp);
	std::vector<std::string> getAllTarget();
	bool new_server(bool overFlag, KXmlAttribute& attr, std::string& err_msg);
#ifdef ENABLE_VH_RUN_AS
	bool cmdForm(KXmlAttribute& attribute, std::string& errMsg);
	KCmdPoolableRedirect* newCmdRedirect(std::map<std::string, std::string>& attribute,
		std::string& errMsg);
	KCmdPoolableRedirect* refsCmdRedirect(const std::string& name);
	bool cmdEnable(const std::string&, bool enable);
	bool delCmd(const std::string& name, std::string& err_msg) {
		return remove_item("cmd", name, err_msg);
	}
	bool addCmd(KCmdPoolableRedirect* as)
	{
		lock.WLock();
		std::map<std::string, KCmdPoolableRedirect*>::iterator it = cmds.find(as->name);
		if (it != cmds.end()) {
			lock.WUnlock();
			return false;
		}
		cmds.insert(std::pair<std::string, KCmdPoolableRedirect*>(as->name, as));
		lock.WUnlock();
		return true;
	}
#endif
	bool apiEnable(const std::string&, bool enable);
	bool delApi(const std::string& name, std::string& err_msg) {
		return remove_item("api", name, err_msg);
	}
	bool apiForm(KXmlAttribute& attribute, std::string& errMsg);
	KSingleAcserver* refsSingleAcserver(const std::string& name);
	KPoolableRedirect* refsAcserver(const std::string& name);
	KRedirect* refsRedirect(const std::string &target);
	KApiRedirect* refsApiRedirect(const std::string& name);
	void clearImportConfig();
	friend class KAccess;
	friend class KHttpManage;
private:
	KSingleAcserver* getSingleAcserver(const std::string& table_name);
#ifdef ENABLE_MULTI_SERVER
	KMultiAcserver* getMultiAcserver(const std::string&  table_name);
	std::map<std::string, KMultiAcserver*> mservers;
#endif
	KPoolableRedirect* getAcserver(const std::string& table_name);
	KApiRedirect* getApiRedirect(const std::string& name);
#ifdef ENABLE_VH_RUN_AS
	KCmdPoolableRedirect* getCmdRedirect(const std::string& name);
	std::map<std::string, KCmdPoolableRedirect*> cmds;
#endif
	std::map<std::string, KSingleAcserver*> acservers;
	std::map<std::string, KApiRedirect*> apis;
	bool remove_item(const std::string& scope, const std::string& name, std::string& err_msg);
	bool new_item(const std::string& scope, bool is_update, KXmlAttribute& attr, std::string& err_msg);
	KRWLock lock;
	KRLocker get_rlocker() {
		return KRLocker(&lock);
	}
	KWLocker get_wlocker() {
		return KWLocker(&lock);
	}
};
void on_server_event(void* data, kconfig::KConfigTree* tree, kconfig::KConfigEvent *ev);
void on_api_event(void* data, kconfig::KConfigTree* tree, kconfig::KConfigEvent* ev);
void on_cmd_event(void* data, kconfig::KConfigTree* tree, kconfig::KConfigEvent* ev);
#endif /*KACSERVERMANAGER_H_*/
