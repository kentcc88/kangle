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
#include <string.h>
#include <stdlib.h>
#include "KAccess.h"
#include <map>
#include "KChain.h"
#include "whm.h"
#include "WhmContext.h"
#include "KAcserverManager.h"
#include "KWriteBackManager.h"
#include "KBaseVirtualHost.h"
#include "http.h"
#include "do_config.h"
#include "KTable.h"
#include "kmalloc.h"
#include "cache.h"
#include "KSrcAcl.h"
#include "KModelManager.h"
#include "KPathAcl.h"
#include "KRegPathAcl.h"
#include "KUrlAcl.h"
#include "KSpeedLimitMark.h"
#include "KGSpeedLimitMark.h"
#include "KFlagMark.h"
#include "KHostAcl.h"
#include "KSelfPortAcl.h"
#include "KDstPortAcl.h"
#include "KMethodAcl.h"
#include "KRewriteMarkEx.h"
#include "KObjFlagAcl.h"
#include "KSelfIpAcl.h"
#include "KRequestHeaderAcl.h"
#include "KResponseHeaderAcl.h"
#include "KContentLengthAcl.h"
#include "KResponseFlagMark.h"
#include "KLoadAvgAcl.h"
#include "KFileAcl.h"
#include "KDirAcl.h"
#include "KFileExeAcl.h"
#include "KTimeAcl.h"
#include "KHttpProxyFetchObject.h"
#include "KVirtualHost.h"
#include "KVirtualHostManage.h"
#include "KFastcgiFetchObject.h"
#include "KRewriteMark.h"
#include "KCacheControlMark.h"
#include "KRedirectMark.h"
#include "KAuthMark.h"
#include "KMultiHostAcl.h"
#include "KSSLSerialAcl.h"
#include "KCdnRewriteMark.h"
#include "KAuthUserAcl.h"
#include "KRefererAcl.h"
#include "KRegFileAcl.h"
#include "kselector.h"
#include "KAddHeaderMark.h"
#include "KRemoveHeaderMark.h"
#include "KReplaceHeaderMark.h"
#include "KReplaceIPMark.h"
#include "KSrcsAcl.h"
#include "KSelfsAcl.h"
#include "KSelfPortsAcl.h"
#include "KIpSpeedLimitMark.h"
#include "KHttp10Mark.h"
#include "KRandAcl.h"
#include "KCloudIpAcl.h"
#ifdef ENABLE_INPUT_FILTER
#include "KParamMark.h"
#include "KPostFileMark.h"
#include "KUploadProgressMark.h"
#endif
#include "KHttpOnlyCookieMark.h"
#include "KTempFileMark.h"
#include "KRemoveParamMark.h"
#include "KHostAliasMark.h"
#include "KFlowMark.h"
#include "KUrlRewriteMark.h"
#include "KUrlRangeMark.h"
#include "KVaryMark.h"
#ifdef ENABLE_FATBOY
#include "KWhiteListModel.h"
#endif
#include "KBlackListMark.h"
#include "KIpRateAcl.h"
#include "KMultiServerMark.h"
#include "KWorkerAcl.h"
#include "KIpUrlRateMark.h"
#include "KIpUrlRateAcl.h"
#include "KUrlRateAcl.h"
#include "KWorkModelAcl.h"
#include "KObjAlwaysOnAcl.h"
#include "KGeoMark.h"
#ifdef WORK_MODEL_TCP
#include "KPortMapMark.h"
#endif

#include "KTcpFetchObject.h"
#include "KQueueMark.h"
#include "KPathSignMark.h"
#include "KStubStatusMark.h"
#include "KMarkMark.h"
#include "KMarkAcl.h"
#include "KCounterMark.h"
#include "KStatusCodeAcl.h"
#include "KStatusCodeMark.h"
#include "KPerIpAcl.h"
#include "KTimeoutMark.h"
#include "KKeepConnectionAcl.h"
#include "KConnectionCloseMark.h"
#include "KMinObjVerifiedMark.h"
#include "KTryFileAcl.h"
#include "KMapRedirectMark.h"
#include "kmalloc.h"
#include "KConfigTree.h"
#ifdef ENABLE_TCMALLOC
#include "google/heap-checker.h"
#endif
using namespace std;

KAccess* kaccess[2] = { 0 };
std::map<KString, KAcl*> KAccess::aclFactorys[2];
std::map<KString, KMark*> KAccess::markFactorys[2];

void bind_access_config(kconfig::KConfigTree* tree, KAccess *access) {
	if (tree->add(_KS(""), access) != nullptr) {
		access->add_ref();
	}
	return;
}

int32_t KAccess::ShutdownMarkModule() {
	int32_t result = 0;
	std::map<KString, KMark*>::iterator it;
	for (int i = 0; i < 2; i++) {
		for (it = KAccess::markFactorys[i].begin(); it != markFactorys[i].end(); it++) {
			result += (*it).second->shutdown();
		}
	}
	return result;
}
KAccess::KAccess(bool is_global, u_short type) {
	rwlock = kfiber_rwlock_init();
	default_jump = NULL;
	default_jump_type = JUMP_ALLOW;
	begin = NULL;
	post_map = NULL;
	string err_msg;
	this->globalFlag = is_global;
	this->type = type;
	ref = 1;
}
KAccess::~KAccess() {
	inter_destroy();
	kfiber_rwlock_destroy(rwlock);
}
void KAccess::inter_destroy() {
	for (auto&& table : tables) {
		table.second->clear();
	}
	tables.clear();
	begin = nullptr;
	post_map = nullptr;
	if (this->default_jump) {
		this->default_jump->release();
		this->default_jump = NULL;
	}
}
void KAccess::clear() {
	auto locker = write_lock();
	inter_destroy();
}
bool KAccess::isGlobal() {
	return globalFlag;
}
bool KAccess::addAclModel(u_short type, KAcl* m, bool replace) {

	if (type > 1) {
		m->add_ref();
		for (u_short i = 0; i < 2; i++) {
			addAclModel(i, m, replace);
			//aclFactorys[i].insert(std::pair<KString,KAcl *>(m->getName(),m));
		}
		return true;
	}
	std::map<KString, KAcl*>::iterator it = aclFactorys[type].find(m->getName());
	if (it != aclFactorys[type].end()) {
		if (!replace) {
			m->release();
			return false;
		}
		(*it).second->release();
		aclFactorys[type].erase(it);
	}
	aclFactorys[type].insert(std::pair<KString, KAcl*>(m->getName(), m));
	return true;
}
bool KAccess::addMarkModel(u_short type, KMark* m, bool replace) {
	if (type > 1) {
		m->add_ref();
		for (u_short i = 0; i < 2; i++) {
			addMarkModel(i, m, replace);
			//markFactorys[i].insert(std::pair<KString,KMark *>(m->getName(),m));
		}
		return true;
	}
	std::map<KString, KMark*>::iterator it = markFactorys[type].find(m->getName());
	if (it != markFactorys[type].end()) {
		if (!replace) {
			m->release();
			return false;
		}
		(*it).second->release();
		markFactorys[type].erase(it);
	}
	markFactorys[type].insert(std::pair<KString, KMark*>(m->getName(), m));
	return true;
}
void KAccess::loadModel() {
#ifdef ENABLE_TCMALLOC
	HeapLeakChecker::Disabler disabler;
#endif
	addAclModel(REQUEST_RESPONSE, new KUrlAcl());
	KAcl* acl = NULL;
	addAclModel(REQUEST_RESPONSE, new KRegPathAcl());
	addAclModel(REQUEST_RESPONSE, new KRegParamAcl());
	addAclModel(REQUEST_RESPONSE, new KPathAcl());
	addAclModel(REQUEST_RESPONSE, new KDstPortAcl());
	addAclModel(REQUEST_RESPONSE, new KMethodAcl());
	addAclModel(REQUEST_RESPONSE, new KSrcAcl());

	addAclModel(REQUEST_RESPONSE, new KSrcsAcl());

	addAclModel(REQUEST, new KRequestHeaderAcl());
	addAclModel(REQUEST, new KHostAcl());
	addAclModel(REQUEST, new KHeaderMapAcl());
	addAclModel(REQUEST, new KWideHostAcl());
	addAclModel(REQUEST, new KMultiHostAcl());
#ifdef ENABLE_SIMULATE_HTTP
	addAclModel(REQUEST_RESPONSE, new KCloudIpAcl());
#endif
#ifndef _WIN32
	addAclModel(REQUEST_RESPONSE, new KLoadAvgAcl());
#endif
	addAclModel(REQUEST, new KTimeAcl());

	acl = new KFileExeAcl();
	addAclModel(REQUEST, acl);
	acl->add_ref();
	addAclModel(RESPONSE, acl);
	acl = new KFileAcl();
	//addAclModel(REQUEST,acl);
	//acl->addRef();
	addAclModel(RESPONSE, acl);
	addAclModel(RESPONSE, new KFileNameAcl());
	addAclModel(RESPONSE, new KDirAcl());
	addAclModel(RESPONSE, new KRegFileAcl());
	addAclModel(RESPONSE, new KRegFileNameAcl());
	addMarkModel(REQUEST, new KSpeedLimitMark());
#ifdef ENABLE_REQUEST_QUEUE
	addMarkModel(REQUEST, new KQueueMark());
	addMarkModel(REQUEST, new KPerQueueMark());
#endif
	addMarkModel(REQUEST, new KGSpeedLimitMark());
	addMarkModel(REQUEST, new KRemoveParamMark());
	addMarkModel(REQUEST, new KHostAliasMark());
	addMarkModel(REQUEST, new KMinObjVerifiedMark());
	addMarkModel(REQUEST_RESPONSE, new KFlagMark());
	addMarkModel(REQUEST, new KRewriteMark());
	addMarkModel(REQUEST, new KRedirectMark());
	addMarkModel(REQUEST, new KMapRedirectMark());
	addMarkModel(REQUEST_RESPONSE, new KCounterMark());
	addMarkModel(REQUEST_RESPONSE, new KAuthMark());
	addMarkModel(REQUEST, new KExtendFlagMark());
	addAclModel(REQUEST, new KPerIpAcl());
#ifdef KSOCKET_SSL
	addAclModel(REQUEST, new KSSLSerialAcl());
#endif

	addAclModel(RESPONSE, new KObjFlagAcl());
	addAclModel(RESPONSE, new KHostAcl());
	addAclModel(RESPONSE, new KWideHostAcl());
	addAclModel(RESPONSE, new KMultiHostAcl());

	addAclModel(RESPONSE, new KResponseHeaderAcl());
	addAclModel(RESPONSE, new KContentLengthAcl());
	addAclModel(RESPONSE, new KStatusCodeAcl());
	addMarkModel(RESPONSE, new KCacheControlMark());
	//addMarkModel(RESPONSE, new KGuestCacheMark());
	//addMarkModel(RESPONSE,new KRegContentMark());
	addMarkModel(RESPONSE, new KResponseFlagMark());
	addMarkModel(RESPONSE, new KExtendFlagMark());
	//addMarkModel(RESPONSE,new KStatusCodeMark());

	acl = new KMarkAcl();
	acl->add_ref();
	addAclModel(REQUEST, acl);
	addAclModel(RESPONSE, acl);

	acl = new KSelfIpAcl();
	addAclModel(REQUEST, acl);
	acl->add_ref();
	addAclModel(RESPONSE, acl);
	acl = new KSelfsAcl();
	addAclModel(REQUEST, acl);
	acl->add_ref();
	addAclModel(RESPONSE, acl);

	acl = new KSelfPortAcl();
	addAclModel(REQUEST, acl);
	acl->add_ref();
	addAclModel(RESPONSE, acl);
	acl = new KSelfPortsAcl();
	addAclModel(REQUEST, acl);
	acl->add_ref();
	addAclModel(RESPONSE, acl);
	//acl = new KVhAcl();
	//addAclModel(REQUEST,acl);
	//acl->addRef();
	//addAclModel(RESPONSE,acl);
	addAclModel(REQUEST_RESPONSE, new KRandAcl());
	addAclModel(REQUEST_RESPONSE, new KAuthUserAcl());
	addAclModel(REQUEST_RESPONSE, new KRegAuthUserAcl());
	addAclModel(REQUEST, new KRefererAcl());
	addAclModel(REQUEST, new KTryFileAcl());
	addMarkModel(REQUEST, new KRewriteMarkEx());
	addMarkModel(REQUEST, new KUrlRewriteMark());
	addMarkModel(REQUEST, new KHostMark());
	addMarkModel(REQUEST, new KHostRewriteMark());
	addMarkModel(REQUEST, new KReplaceIPMark());
	//addMarkModel(REQUEST,new KSelfIPMark());
	addMarkModel(REQUEST, new KParentMark());
	addMarkModel(REQUEST, new KAddResponseHeaderMark());
#ifdef ENABLE_INPUT_FILTER
	addMarkModel(REQUEST, new KParamMark());
	addMarkModel(REQUEST, new KParamCountMark());
	addMarkModel(REQUEST, new KPostFileMark());
	addMarkModel(RESPONSE, new KHttpOnlyCookieMark());
#endif
	addMarkModel(RESPONSE, new KCookieMark());
	//{{ent
#ifdef KANGLE_ENT
	addAclModel(REQUEST_RESPONSE, new KWorkModelAcl());
#ifdef ENABLE_FATBOY
	addAclModel(REQUEST_RESPONSE, new KWhiteListAcl());
	addMarkModel(REQUEST_RESPONSE, new KWhiteListMark);
#endif
	addAclModel(REQUEST, new KIpRateAcl());
	addMarkModel(REQUEST, new KMultiServerMark());
	//addMarkModel(RESPONSE,new KReplaceUrlMark());
	//addMarkModel(REQUEST_RESPONSE,new KFixHeaderMark());
	addMarkModel(REQUEST, new KGeoMark());
#endif
#ifdef ENABLE_BLACK_LIST
	addMarkModel(REQUEST, new KCheckBlackListMark());
	addMarkModel(REQUEST, new KBlackListMark());
	addMarkModel(REQUEST, new KIpUrlRateMark());
	addAclModel(REQUEST_RESPONSE, new KIpUrlRateAcl());
	addAclModel(REQUEST_RESPONSE, new KUrlRateAcl());
#endif
#ifdef WORK_MODEL_TCP
	addMarkModel(REQUEST, new KPortMapMark());
#endif
	addAclModel(RESPONSE, new KObjAlwaysOnAcl());
	//}}
	addMarkModel(REQUEST, new KPathSignMark());
	addAclModel(REQUEST_RESPONSE, new KListenPortsAcl());
	addMarkModel(REQUEST, new KFlowMark());
	addMarkModel(RESPONSE, new KVaryMark());
	addMarkModel(REQUEST, new KIpSpeedLimitMark());
	addMarkModel(REQUEST_RESPONSE, new KAddHeaderMark());
	addMarkModel(REQUEST_RESPONSE, new KRemoveHeaderMark());
	addMarkModel(REQUEST_RESPONSE, new KReplaceHeaderMark());
	addMarkModel(REQUEST_RESPONSE, new KTimeoutMark());
	//addMarkModel(RESPONSE,new KFooterMark());
	//addMarkModel(RESPONSE,new KReplaceContentMark());
	//addMarkModel(REQUEST,new KUrlRangeMark());
	addMarkModel(REQUEST, new KMarkMark());
#ifdef ENABLE_STAT_STUB
	addMarkModel(REQUEST, new KStubStatusMark());
#endif
	addMarkModel(RESPONSE, new KMarkMark());
	addMarkModel(REQUEST_RESPONSE, new KConnectionCloseMark());
}
kgl_jump_type KAccess::checkPostMap(KHttpRequest* rq, KHttpObject* obj, KFetchObject** fo) {
	if (post_map == NULL) {
		return JUMP_ALLOW;
	}
	kgl_jump_type jumpType = default_jump_type;
	unsigned checked_table = 0;
	KJump* jump = default_jump;
	kfiber_rwlock_rlock(rwlock);
	if (post_map) {
		jumpType = post_map->match(rq, obj, checked_table, &jump, fo);
		if (jumpType == JUMP_RETURN) {
			jumpType = default_jump_type;
		}
	}
	kfiber_rwlock_runlock(rwlock);
	if (jumpType == JUMP_ALLOW) {
		return JUMP_ALLOW;
	}
	return JUMP_DENY;
}
kgl_jump_type KAccess::check(KHttpRequest* rq, KHttpObject* obj, KFetchObject** fo) {
	kgl_jump_type jumpType = default_jump_type;
	unsigned checked_table = 0;
	KJump* jump = default_jump;
	kfiber_rwlock_rlock(rwlock);
	if (begin) {
		jumpType = begin->match(rq, obj, checked_table, &jump, fo);
		if (fo && *fo) {
			kfiber_rwlock_runlock(rwlock);
			if ((*fo)->before_cache()) {
				return JUMP_DENY;
			}
			return JUMP_ALLOW;
		}
		if (jumpType == JUMP_RETURN || jumpType == JUMP_CONTINUE) {
			jumpType = default_jump_type;
			jump = default_jump;
		}
	}
	switch (jumpType) {
	case JUMP_SERVER:
	{
		if (fo) {
			assert(!rq->has_final_source());
			KPoolableRedirect* as = (KPoolableRedirect*)jump;
			KRedirectSource* fo2 = as->makeFetchObject(rq, NULL);
			as->add_ref();
			fo2->bind_base_redirect(new KBaseRedirect(as, KConfirmFile::Never));
			jumpType = JUMP_ALLOW;
			*fo = fo2;
		}
		break;
	}
#ifdef ENABLE_WRITE_BACK
	case JUMP_WBACK:
		if (jump && fo) {
			KWriteBack* wb = (KWriteBack*)jump;
			wb->buildRequest(rq, fo);
		}
		jumpType = JUMP_DENY;
		break;
#endif
	case JUMP_PROXY:
		jumpType = JUMP_ALLOW;
		assert(!rq->has_final_source());
		if (fo) {
#ifdef HTTP_PROXY
			if (rq->sink->data.meth == METH_CONNECT) {
				*fo = new KConnectProxyFetchObject();
				break;
			}
#endif
#ifdef ENABLE_PROXY_PROTOCOL
			if (KBIT_TEST(rq->GetWorkModel(), WORK_MODEL_PROXY | WORK_MODEL_SSL_PROXY)) {
				*fo = new KTcpFetchObject(false);
				break;
			}
#endif
			* fo = new KHttpProxyFetchObject();
		}
		break;
	}
	kfiber_rwlock_runlock(rwlock);
	return jumpType;
}
KSafeTable KAccess::getTable(const KString &table_name) {
	auto it = tables.find(table_name);
	if (it != tables.end()) {
		return (*it).second;
	}
	return KSafeTable();
}
KString KAccess::htmlAccess(const char* vh) {
	KStringBuf s;
	if (*vh == '\0') {
		s << "<html><LINK href=/main.css type='text/css' rel=stylesheet>\n";
		s << "<body>";
	}
	s << "<div>";
	s << "ref:" << katom_get((void*)&ref) << " ";
	s << (type == REQUEST ? klang["lang_requestAccess"] : klang["lang_responseAccess"]) << " " << LANG_ACCESS_FIRST << ":";
	{
		auto locker = read_lock();		
		buildChainAction(default_jump_type, default_jump, s);
		s << "</div>";
		for (auto it = tables.begin(); it != tables.end(); ++it) {
			s << "<div>";
			(*it).second->htmlTable(s, vh, type);
			s << "</div>";
		}
	}
	if (*vh == '\0') {
		s << endTag();
	}
	return s.str();
}
bool KAccess::parseChainAction(const KString& action, kgl_jump_type& jumpType,KString& jumpName) {
	if (strcasecmp(action.c_str(), "deny") == 0) {
		jumpType = JUMP_DENY;
	}
	if (strcasecmp(action.c_str(), "drop") == 0) {
		jumpType = JUMP_DROP;
	}
	if (strcasecmp(action.c_str(), "allow") == 0) {
		jumpType = JUMP_ALLOW;
	}
	if (strcasecmp(action.c_str(), "continue") == 0) {
		jumpType = JUMP_CONTINUE;
	}
	if (strcasecmp(action.c_str(), "return") == 0 || strcasecmp(action.c_str(), "default") == 0) {
		jumpType = JUMP_RETURN;
	}
	if (strncasecmp(action.c_str(), "table:", 6) == 0) {
		jumpType = JUMP_TABLE;
		jumpName = action.substr(6);
	}
	if (strncasecmp(action.c_str(), "wback:", 6) == 0) {
		jumpType = JUMP_WBACK;
		jumpName = action.substr(6);
	}
	if (strcasecmp(action.c_str(), "proxy") == 0) {
		jumpType = JUMP_PROXY;
	}
	if (strncasecmp(action.c_str(), "server:", 7) == 0) {
		jumpType = JUMP_SERVER;
		jumpName = action.substr(7);
	}
	//user access not support this action
	if (isGlobal()) {
		if (strncasecmp(action.c_str(), "vhs", 4) == 0) {
			jumpType = JUMP_VHS;
		}
		if (strncasecmp(action.c_str(), "cgi:", 4) == 0) {
			jumpType = JUMP_CGI;
			jumpName = action.substr(4);
		}
		if (strncasecmp(action.c_str(), "api:", 4) == 0) {
			jumpType = JUMP_API;
			jumpName = action.substr(4);
		}
		if (strncasecmp(action.c_str(), "cmd:", 4) == 0) {
			jumpType = JUMP_CMD;
			jumpName = action.substr(4);
		}
		if (strncasecmp(action.c_str(), "dso:", 4) == 0) {
			jumpType = JUMP_DSO;
			jumpName = action.substr(4);
		}
	}
	return true;
}
void KAccess::buildChainAction(kgl_jump_type jumpType, KJump* jump, KWStream & s) {
	bool jname = false;
	switch (jumpType) {
	case JUMP_DROP:
		s << "drop";
		break;
	case JUMP_DENY:
		s << "deny";
		break;
	case JUMP_ALLOW:
		s << "allow";
		break;
	case JUMP_CONTINUE:
		s << "continue";
		break;
	case JUMP_RETURN:
		s << "return";
		break;
	case JUMP_PROXY:
		s << "proxy";
		break;
	case JUMP_SERVER:
		s << "server:";
		jname = true;
		break;
	case JUMP_WBACK:
		s << "wback:";
		jname = true;
		break;
	case JUMP_TABLE:
		s << "table:";
		jname = true;
		break;
	}
	if (jname && jump) {
		s << jump->name;
	}
}
void KAccess::setChainAction(kgl_jump_type& jump_type, KJump** jump, const KString &name) {
	if (*jump) {
		(*jump)->release();
		*jump = nullptr;
	}
	switch (jump_type) {
#ifdef ENABLE_MULTI_TABLE
	case JUMP_TABLE:
	{
		if (name[0] == '~') {
			*jump = (KJump*)kaccess[type]->getTable(name).release();
		} else {
			*jump = (KJump*)getTable(name).release();
		}
		if (*jump == nullptr) {
			fprintf(stderr, "cann't get table name=[%s]\n", name.c_str());
			jump_type = JUMP_DENY;
			throw KXmlException("cann't found table");			
		}
		break;
	}
#endif
	case JUMP_SERVER:
		*jump = (KJump*)conf.gam->refsAcserver(name);
		if (*jump == NULL) {
			klog(KLOG_ERR, "cann't get server name=[%s]\n", name.c_str());
			jump_type = JUMP_DENY;
			throw KXmlException("cann't found server");
		}
		break;
#ifdef ENABLE_WRITE_BACK
	case JUMP_WBACK:
		*jump = (KJump*)writeBackManager.refsWriteBack(name);
		if (*jump == NULL) {
			klog(KLOG_ERR, "cann't get writeback name=[%s]\n", name.c_str());
			jump_type = JUMP_DENY;
			throw KXmlException("cann't found writeback");
		}
		break;
#endif
	}
	return;
}
std::vector<KString> KAccess::getTableNames(KString skipName, bool global) {
	std::vector<KString> table_names;
	for (auto it = tables.begin(); it != tables.end(); it++) {
		if ((skipName.size() == 0 || (skipName.size() > 0 && skipName != (*it).first)) && (*it).first != BEGIN_TABLE) {
			if (global) {
				if ((*it).first[0] == '~') {
					table_names.push_back((*it).first);
				}
				continue;
			}
			table_names.push_back((*it).first);
		}
	}
	return table_names;
}
void KAccess::listTable(KVirtualHostEvent* ctx) {
	kfiber_rwlock_rlock(rwlock);
	for (auto it = tables.begin(); it != tables.end(); it++) {
		ctx->add("table", (*it).first.c_str());
	}
	kfiber_rwlock_runlock(rwlock);
}
void KAccess::parse_config(const KXmlAttribute& attr) {
	auto locker = write_lock();
	KString jump_name;
	parseChainAction(attr["action"], default_jump_type, jump_name);
	setChainAction(this->default_jump_type, &this->default_jump, jump_name);
}
bool KAccess::on_config_event(kconfig::KConfigTree* tree, kconfig::KConfigEvent* ev) {
	switch (ev->type) {
	case kconfig::EvUpdate:
	case kconfig::EvNew:
	{
		parse_config(ev->get_xml()->attributes());
		break;
	}
	case kconfig::EvRemove:
	{
		if (!isGlobal()) {
			auto type = this->get_type();
			auto acc = static_cast<KAccess*>(tree->unbind());
			assert(acc && this == acc);
			defer(acc->release(););
			auto vh = static_cast<KVirtualHost*>(tree->parent->ls);
			if (vh) {
				auto locker = vh->get_locker();
				assert(vh->access[type].get() == this);
				vh->access[type] = nullptr;
			}
			return true;
		}
		auto locker = write_lock();
		KString jump_name;
		parseChainAction("allow", default_jump_type, jump_name);
		setChainAction(this->default_jump_type, &this->default_jump, jump_name);
		break;
	}
	case kconfig::EvSubDir|kconfig::EvUpdate:
	{
		auto xml = ev->get_xml();
		if (xml->is_tag(_KS("table"))) {			
			auto body = xml->get_first();
			if (!body) {
				return false;
			}
			auto locker = write_lock();
			auto it = tables.find(body->attributes["name"]);
			if (it == tables.end()) {
				klog(KLOG_ERR, "cann't find table name [%s]\n", body->attributes("name"));
				return false;
			}
			if (!(*it).second->parse_config(this, body)) {
				klog(KLOG_ERR, "cann't parse table config\n");
				return false;
			}
		}
		return true;
	}
	case kconfig::EvNew| kconfig::EvSubDir:
	{
		auto xml = ev->get_xml();
		if (xml->is_tag(_KS("table"))) {
			auto body = xml->get_first();
			if (!body) {
				return false;
			}
			auto locker = write_lock();
			KSafeTable table(new KTable(this, body->attributes["name"]));
			//printf("new table name=[%s]\n", table->name.c_str());
			if (!table->parse_config(this, body)) {
				klog(KLOG_ERR, "cann't parse table config\n");
				return false;
			}
			if (table->name == BEGIN_TABLE) {
				begin = table;
			} else {
				if (type == RESPONSE && table->name == "POSTMAP") {
					post_map = table;
				}
			}
			tree->bind(static_cast<KTable*>(table->add_ref()));
			tables[table->name] = std::move(table);
			return true;
		}
		break;
	}
	case kconfig::EvRemove | kconfig::EvSubDir:
	{
		auto xml = ev->get_xml();
		if (xml->is_tag(_KS("table"))) {			
			auto body = xml->get_first();
			assert(body);
			if (!body) {
				return false;
			}
			auto locker = write_lock();
			KSafeTable table(static_cast<KTable *>(tree->unbind()));
			//printf("remove table name=[%s]\n", table->name.c_str());
			tables.erase(table->name);
			table->clear();
			return true;
		}
		break;
	}
	}
	return true;
}