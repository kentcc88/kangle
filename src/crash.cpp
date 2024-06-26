#include "kforwin32.h"
#include <sstream>
#include <string.h>
#include "do_config.h"
#include "directory.h"
#include "utils.h"
#include "zlib.h"
#include "KSimulateRequest.h"

#ifdef _WIN32
#pragma comment(lib,"netapi32.lib")
#endif
#if 0
using namespace std;
struct kgl_upload_context
{
	KFile fp;
	std::string filename;
	std::string bounder;
	kgl_pool_t* pool;
	char* data;
	int data_left;
};
static int WINAPI upload_read(void* async_http_arg, char* buf, int len) {
	kgl_upload_context* file_ctx = (kgl_upload_context*)async_http_arg;
restart:
	if (file_ctx->data_left>0) {
		int got = KGL_MIN(file_ctx->data_left, len);
		memcpy(buf, file_ctx->data, got);
		file_ctx->data += got;
		file_ctx->data_left -= got;
		if (got > 0) {
			return got;
		}
	}
	if (!file_ctx->fp.opened()) {
		return -1;
	}		
	int got = file_ctx->fp.read((char*)buf, len);
	if (got > 0) {
		return got;
	}
	file_ctx->fp.close();
	file_ctx->data = (char*)kgl_pnalloc(file_ctx->pool, file_ctx->bounder.size() + 8);
	char* hot = file_ctx->data;
	memcpy(hot, kgl_expand_string("\r\n--"));
	hot += 4;
	memcpy(hot, file_ctx->bounder.c_str(), file_ctx->bounder.size());
	hot += file_ctx->bounder.size();
	memcpy(hot, kgl_expand_string("--\r\n"));
	file_ctx->data_left = (int)file_ctx->bounder.size() + 8;
	goto restart;
}
static void upload_close(kgl_upload_context* file_ctx, int exptected_done) {
	file_ctx->fp.close();
	unlink(file_ctx->filename.c_str());
	kgl_destroy_pool(file_ctx->pool);
	delete file_ctx;
}
static int WINAPI upload_write(void* async_http_arg, const char* data, int len) {
	if (data == NULL) {
		upload_close((kgl_upload_context *)async_http_arg, len);
		return 0;
	}
	fwrite(data, 1, len, stdout);
	return 0;
}

void upload_dmp_file(const char* file, const char* url) {
	//printf("upload core file =[%s]\n",file);
	//kgl_upload_context *file_ctx = new kgl_upload_context;
	KFile fp;
	if (!fp.open(file, fileRead)) {
		return;
	}
	stringstream gz_file_name;
	gz_file_name << file << ".gz";
	gzFile gz = gzopen(gz_file_name.str().c_str(), "wb");
	if (gz == NULL) {
		return;
	}
	char buf[8192];
	for (;;) {
		int n = fp.read(buf, sizeof(buf));
		if (n <= 0) {
			break;
		}
		if (gzwrite(gz, buf, n) < 0) {
			break;
		}
	}
	gzclose(gz);
	kgl_upload_context* file_ctx = new kgl_upload_context;
	file_ctx->pool = kgl_create_pool(512);
	file_ctx->filename = gz_file_name.str();
	if (!file_ctx->fp.open(file_ctx->filename.c_str(), fileRead)) {
		delete file_ctx;
		return;
	}
	kgl_async_http ctx;
	memset(&ctx, 0, sizeof(ctx));
	ctx.arg = file_ctx;
	ctx.post = upload_read;
	ctx.body = upload_write;
	ctx.meth = (char*)"POST";
	ctx.url = (char*)url;
	ctx.queue = "upload_1_0";
	stringstream s;
	std::stringstream bounder;
	bounder << "---------------------------4664151417711";
	bounder << rand();
	file_ctx->bounder = bounder.str();


	s << "--" << file_ctx->bounder << "\r\nContent-Disposition: form-data; name=\"file\"; filename=\"" << file_ctx->filename << "\"\r\n";
	s << "Content-Type: text/plain" << "\r\n\r\n";
	file_ctx->data_left = (int)s.str().size();
	file_ctx->data = (char*)kgl_pnalloc(file_ctx->pool, file_ctx->data_left);
	memcpy(file_ctx->data, s.str().c_str(), file_ctx->data_left);

	s.str("");
	s << "multipart/form-data; boundary=" << file_ctx->bounder;
	KHttpHeader* header = new_pool_http_header(kgl_expand_string("Content-Type"), s.str().c_str(), (int)s.str().size(), (kgl_malloc)kgl_pnalloc, file_ctx->pool);
	ctx.rh = header;
	ctx.post_len = (int)file_ctx->fp.getFileSize() + file_ctx->data_left + file_ctx->bounder.size() + 8;
	if (kgl_simuate_http_request(&ctx) != 0) {
		upload_close(file_ctx, -1);
	}
}
int crash_report(const char* file, void* param) {
	char* path = (char*)param;
#ifdef _WIN32
	const char* p = strrchr(file, '.');
	if (p == NULL) {
		return 0;
	}
	if (!(strcasecmp(p + 1, "leak") == 0 || strcasecmp(p + 1, "dmp") == 0)) {
		return 0;
	}
#else
	if (strncasecmp(file, "core", 4) != 0) {
		return 0;
	}
#endif
	stringstream s;
	s << path << file;
	stringstream u;
	u << "http://crash.kangleweb.net/v2.php?version=" << VERSION << "&type=" << getServerType() << "&f=";
	upload_dmp_file(s.str().c_str(), u.str().c_str());
	unlink(s.str().c_str());
	return 0;
}
#endif
KTHREAD_FUNCTION crash_report_thread(void* arg) {
#ifdef _WIN32
	char buf[512];
	::GetModuleFileName(NULL, buf, sizeof(buf) - 1);
	char* p = strrchr(buf, '\\');
	if (p) {
		p++;
		*p = '\0';
	}
#else
	const char* buf = conf.path.c_str();
#endif
	//list_dir(buf, crash_report, (void*)buf);
	KTHREAD_RETURN;
}