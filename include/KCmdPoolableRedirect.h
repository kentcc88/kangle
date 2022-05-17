/*
 * KCmdPoolableRedirect.h
 *
 *  Created on: 2010-8-26
 *      Author: keengo
 */
#ifndef KCMDPOOLABLEREDIRECT_H_
#define KCMDPOOLABLEREDIRECT_H_
#include <map>
#include <vector>
#include <string.h>
#include "global.h"
#ifdef ENABLE_VH_RUN_AS
#include "KVirtualHost.h"
#include "KAcserver.h"
#include "KProcessManage.h"
#include "KExtendProgram.h"
#include "ksocket.h"
#include "KListenPipeStream.h"


/*
 * 命令扩展。
 * 支持进程模型为SP和MP
 * 支持多种协议，HTTP,AJP,FASTCGI等等。
 *
 */
class KCmdPoolableRedirect: public KPoolableRedirect, public KExtendProgram {
public:
	KCmdPoolableRedirect();
	virtual ~KCmdPoolableRedirect();
	unsigned getPoolSize() {
		return 0;
	}
	const char *getName() {
		return name.c_str();
	}
	KUpstream* GetUpstream(KHttpRequest* rq) override;
	void parseConfig(std::map<std::string, std::string> &attribute) override;

	KProcessManage *getProcessManage() {
		return static_cast<KProcessManage *>(&pm);
	}
	bool Exec(KVirtualHost* vh, KListenPipeStream* st, bool isSameRunning);
	//KTcpUpstream *createPipeStream(KVirtualHost *vh, KListenPipeStream *st, std::string &unix_path,bool isSameRunning);
	bool parseEnv(std::map<std::string, std::string> &attribute) override;
	void buildXML(std::stringstream &s) override;
	const char *getType() override {
		return "cmd";
	}
	void LockCommand()
	{
		kfiber_mutex_lock(lock);
	}
	void UnlockCommand()
	{
		kfiber_mutex_unlock(lock);
	}
	bool isChanged(KPoolableRedirect *rd) override;
	void set_proto(Proto_t proto) override;
	bool chuser;
	int worker;
	int port;
	int sig;
	friend class KSPCmdGroupConnection;	
	std::string cmd;
private:
	bool setWorkType(const char *typeStr,bool changed);
	kfiber_mutex* lock;
	KCmdProcessManage pm;
};
#endif
#endif /* KCMDPOOLABLEREDIRECT_H_ */
