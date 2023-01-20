#include "KDirectoryFetchObject.h"
#include "KHttpObject.h"
#include "http.h"
#include <sstream>
using namespace std;
KGL_RESULT KPrevDirectoryFetchObject::Open(KHttpRequest* rq, kgl_input_stream* in, kgl_output_stream* out)
{
	KHttpObject* obj = rq->ctx.obj;
	KStringBuf new_path2;
	new_path2 << rq->sink->data.raw_url->path << "/";
	if (rq->sink->data.raw_url->param && *rq->sink->data.raw_url->param) {
		new_path2 << "?" << rq->sink->data.raw_url->param;
	}
	push_redirect_header(rq, new_path2.getString(), new_path2.getSize(), STATUS_FOUND);
	rq->start_response_body(0);
	return KGL_OK;
}
KDirectoryFetchObject::KDirectoryFetchObject()
{
#ifdef _WIN32
	dp = INVALID_HANDLE_VALUE;
#else
	dp = NULL;
#endif
}
KDirectoryFetchObject::~KDirectoryFetchObject()
{
#ifdef _WIN32
	if (dp != INVALID_HANDLE_VALUE) {
		FindClose(dp);
	}
#else
	if (dp) {
		closedir(dp);
	}
#endif	
}
KGL_RESULT KDirectoryFetchObject::Open(KHttpRequest* rq, kgl_input_stream* in, kgl_output_stream* out)
{
#ifndef _WIN32
	assert(dp == NULL);
	dp = opendir(rq->file->getName());
	if (dp == NULL) {
		return out->f->error(out->ctx, rq, STATUS_NOT_FOUND, _KS("cann't open dir"));
	}
#else
	KBIT_SET(rq->ctx.obj->index.flags, ANSW_LOCAL_SERVER);
	assert(dp == INVALID_HANDLE_VALUE);
	stringstream dir;
	dir << rq->file->getName() << "\\*";
	closeExecLock.Lock();
	dp = FindFirstFile(dir.str().c_str(), &FileData);
	if (dp == INVALID_HANDLE_VALUE) {
		closeExecLock.Unlock();
		return out->f->error(out->ctx,  STATUS_NOT_FOUND, _KS("cann't open dir"));
	}
	kfile_close_on_exec(dp, true);
	closeExecLock.Unlock();
#endif
	out->f->write_status(out->ctx,  STATUS_OK);
	out->f->write_header(out->ctx,  kgl_header_content_type, _KS("text/html"));
	kgl_response_body body;
	KGL_RESULT ret = out->f->write_header_finish(out->ctx, -1, &body);
	if (ret != KGL_OK) {
		return ret;
	}
	//	KBuffer buffer;
	body.f->write(body.ctx, _KS(
		"<html><head><title>"
		"</title></head><body>\n"
		"<a href='..'>[Parent directory]</a><hr>"
		"<table><tr><td>Name</td><td>Size</td><td>Last modified</td></tr>\n"
	));
#ifndef _WIN32
	for (;;) {
		dirent* fp = readdir(dp);
		if (fp == NULL) {
			break;
		}
		if (strcmp(fp->d_name, ".") == 0 || strcmp(fp->d_name, "..") == 0) {
			continue;
		}
		ret = Write(rq, &body, fp->d_name);
		if (ret != KGL_OK) {
			goto clean;
		}
	}
#else
	for (;;) {
		if ((strcmp(FileData.cFileName, ".") == 0) || (strcmp(FileData.cFileName, "..") == 0)) {
			goto next_file;
		}
		ret = Write(rq, &body, FileData.cFileName);
		if (ret != KGL_OK) {
			goto clean;
		}
	next_file:
		if (!FindNextFile(dp, &FileData)) {
			if (GetLastError() == ERROR_NO_MORE_FILES) {
				break;
			}

		}
	}
#endif
	body.f->write(body.ctx, _KS("</table><hr>Generated by "));
	body.f->write(body.ctx, conf.serverName, conf.serverNameLength);
	ret = body.f->write(body.ctx, _KS("</body></html>"));
clean:
	return body.f->close(body.ctx, ret);

}
KGL_RESULT KDirectoryFetchObject::Write(KHttpRequest* rq, kgl_response_body* out, const char* path)
{
	struct _stat64 sbuf;
	stringstream f;
	f << rq->file->getName() << "/" << path;
	if (_stati64(f.str().c_str(), &sbuf) != 0) {
		return KGL_OK;
	}
	bool isdir = false;
	if (S_ISDIR(sbuf.st_mode)) {
		isdir = true;
	}
	out->f->write(out->ctx,  _KS("<tr><td>"));
	if (isdir) {
		out->f->write(out->ctx, _KS("["));
	}
	out->f->write(out->ctx, _KS("<a href='"));
	std::string encode_path = url_encode(path);
	out->f->write(out->ctx, encode_path.c_str(), (int)encode_path.size());
	if (isdir) {
		out->f->write(out->ctx, _KS("/"));
	}
	out->f->write(out->ctx, _KS("'>"));
	out->f->write(out->ctx, path, (int)strlen(path));
	out->f->write(out->ctx, _KS("</a>"));
	if (isdir) {
		out->f->write(out->ctx, _KS("]"));
	}
	out->f->write(out->ctx, _KS("</td><td>"));
	f.str("");
	if (isdir) {
		//s << "<td colspan=2>&lt;DIR&gt;</td>");
		f << "-";
	} else {
		f << sbuf.st_size;
	}
	out->f->write(out->ctx, f.str().c_str(), (int)f.str().size());
	out->f->write(out->ctx, _KS("</td><td>"));
	char tmp[27];
	make_last_modified_time(&sbuf.st_mtime, tmp, 27);
	out->f->write(out->ctx, tmp, (int)strlen(tmp));
	return out->f->write(out->ctx, _KS("</td></tr>\n"));
}
