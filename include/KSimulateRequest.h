#ifndef KSIMULATEREQUEST_H
#define KSIMULATEREQUEST_H
#include "global.h"
#include "ksocket.h"
#include "KHttpHeaderManager.h"
#include "ksapi.h"
#include "kconnection.h"
#include "KHttpStream.h"
#include "KFileStream.h"
#include "KSink.h"
#include "kfiber.h"
#ifdef ENABLE_SIMULATE_HTTP
KHttpRequest *kgl_create_simulate_request(kgl_async_http *ctx);
int kgl_start_simulate_request(KHttpRequest *rq, kfiber** fiber = NULL);
int kgl_simuate_http_request(kgl_async_http *ctx, kfiber** fiber = NULL);
class KSimulateSink : public KSink
{
public:
	KSimulateSink();
	~KSimulateSink();

	bool SetTransferChunked()
	{
		return false;
	}
	bool internal_response_status(uint16_t status_code)
	{
		this->status_code = status_code;
		return true;
	}
	bool response_header(const char *name, int name_len, const char *val, int val_len)
	{
		KHttpHeader *header = new_pool_http_header(c->pool, name, name_len, val, val_len);
		if (header) {
			header_manager.Append(header);
		}
		return true;
	}
	bool ResponseConnection(const char *val, int val_len)
	{
		return false;
	}
	//返回头长度,-1表示出错
	int StartResponseBody(int64_t body_size)
	{
		if (header) {
			header(arg, status_code, header_manager.header);
		}
		return 0;
	}
	bool IsLocked()
	{
		return false;
	}
	bool read_hup(void *arg, result_callback result)
	{
		return false;
	}
	void RemoveReadHup()
	{

	}
	int internal_read(char *buf,int len)
	{
		if (post) { 
			return post(arg, buf, len);
		}
		return -1;
	}
	bool HasHeaderDataToSend()
	{
		return false;
	}
	int internal_write(WSABUF *buf, int bc)
	{
		if (body) {
			if (body(arg, (char*)buf[0].iov_base, buf[0].iov_len) == 0) {
				return buf[0].iov_len;
			}
		}
		return -1;
	}
	bool get_self_addr(sockaddr_i *addr)
	{
		kgl_memcpy(addr, &c->addr, sizeof(sockaddr_i));
		return true;
	}
	int end_request();
	void AddSync()
	{

	}
	void RemoveSync()
	{

	}
	void Shutdown()
	{

	}
	kconnection *GetConnection()
	{
		return c;
	}
	void SetTimeOut(int tmo_count)
	{

	}
	int GetTimeOut()
	{
		return c->st.tmo;
	}
	void Flush()
	{

	}
	http_header_hook header;
	http_post_hook post;
	http_body_hook body;

	KHttpHeaderManager header_manager;
	char *host;
	uint16_t port;
	uint16_t status_code;
	int life_time;
	void *arg;
	kconnection *c;	
protected:
	void SetReadDelay(bool delay_flag)
	{

	}
	void SetWriteDelay(bool delay_flag)
	{

	}

};
void async_download(const char* url, const char* file, result_callback cb, void* arg);
int kgl_async_download(const char *url, const char *file, int *status);
bool test_simulate_request();
#endif
#endif
