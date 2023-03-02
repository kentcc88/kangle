/*
 * KSingleAcServer.cpp
 *
 *  Created on: 2010-6-4
 *      Author: keengo
 */
#include "do_config.h"
#include "KSingleAcserver.h"
#include "KAsyncFetchObject.h"
#include "HttpFiber.h"

KSingleAcserver::KSingleAcserver(KSockPoolHelper *nodes) : KPoolableRedirect("")
{
	sockHelper = nodes;
}
KSingleAcserver::KSingleAcserver(const std::string &name) : KPoolableRedirect(name) {
	sockHelper = new KSockPoolHelper;
}
KSingleAcserver::~KSingleAcserver() {
	sockHelper->release();
}
bool KSingleAcserver::parse_config(khttpd::KXmlNode* node) {
	if (!KPoolableRedirect::parse_config(node)) {
		return false;
	}
	return sockHelper->parse(node->attributes());
}
void KSingleAcserver::set_proto(Proto_t proto)
{
	this->proto = proto;
	sockHelper->set_tcp(kangle::is_upstream_tcp(proto));
}
KUpstream* KSingleAcserver::GetUpstream(KHttpRequest* rq)
{
	rq->ctx.upstream_sign = sockHelper->sign;
	return sockHelper->get_upstream(rq->get_upstream_flags(), rq->sink->data.raw_url->host);
}
bool KSingleAcserver::setHostPort(std::string host, const char *port) {
	return sockHelper->setHostPort(host , port);
}
void KSingleAcserver::buildXML(std::stringstream &s) {
	s << "\t<server name='" << name << "' proto='";
	s << KPoolableRedirect::buildProto(proto);
	s << "'";
	std::map<std::string, std::string> params;
	sockHelper->build(params);
	build_xml(params, s);
	s << "/>\n";
}
