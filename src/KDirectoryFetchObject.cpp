#include "KDirectoryFetchObject.h"
#include "KHttpObject.h"
#include "http.h"
#include <sstream>
using namespace std;
KGL_RESULT KPrevDirectoryFetchObject::Open(KHttpRequest* rq, kgl_input_stream* in, kgl_output_stream* out)
{
	KHttpObject* obj = rq->ctx->obj;
	KStringBuf new_path2;
	new_path2 << rq->sink->data.raw_url->path << "/";
	if (rq->sink->data.raw_url->param && *rq->sink->data.raw_url->param) {
		new_path2 << "?" << rq->sink->data.raw_url->param;
	}
	push_redirect_header(rq, new_path2.getString(), new_path2.getSize(), STATUS_FOUND);
	rq->startResponseBody(0);
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
		return out->f->write_message(out, rq, KGL_MSG_ERROR, "cann't open dir", STATUS_NOT_FOUND);
	}
#else
	KBIT_SET(rq->ctx->obj->index.flags, ANSW_LOCAL_SERVER);
	assert(dp == INVALID_HANDLE_VALUE);
	stringstream dir;
	dir << rq->file->getName() << "\\*";
	closeExecLock.Lock();
	dp = FindFirstFile(dir.str().c_str(), &FileData);
	if (dp == INVALID_HANDLE_VALUE) {
		closeExecLock.Unlock();
		return out->f->write_message(out, rq, KGL_MSG_ERROR, "cann't open dir", STATUS_NOT_FOUND);
	}
	kfile_close_on_exec(dp, true);
	closeExecLock.Unlock();
#endif
	out->f->write_status(out, rq, STATUS_OK);
	out->f->write_header(out, rq, kgl_header_content_type, _KS("text/html"));
	KGL_RESULT ret = out->f->write_header_finish(out, rq);
	if (ret != KGL_OK) {
		return ret;
	}
	//	KBuffer buffer;
	out->f->write_body(out, rq, _KS(
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
		ret = Write(rq, out, fp->d_name);
		if (ret != KGL_OK) {
			goto clean;
		}
	}
#else
	for (;;) {
		if ((strcmp(FileData.cFileName, ".") == 0) || (strcmp(FileData.cFileName, "..") == 0)) {
			goto next_file;
		}
		ret = Write(rq, out, FileData.cFileName);
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
	out->f->write_body(out, rq, _KS("</table><hr>Generated by "));
	out->f->write_body(out, rq, conf.serverName, conf.serverNameLength);
	ret = out->f->write_body(out, rq, _KS("</body></html>"));
clean:
	return out->f->write_end(out, rq, ret);

}
KGL_RESULT KDirectoryFetchObject::Write(KHttpRequest* rq, kgl_output_stream* out, const char* path)
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
	out->f->write_body(out, rq, _KS("<tr><td>"));
	if (isdir) {
		out->f->write_body(out, rq, _KS("["));
	}
	out->f->write_body(out, rq, _KS("<a href='"));
	std::string encode_path = url_encode(path);
	out->f->write_body(out, rq, encode_path.c_str(), (int)encode_path.size());
	if (isdir) {
		out->f->write_body(out, rq, _KS("/"));
	}
	out->f->write_body(out, rq, _KS("'>"));
	out->f->write_body(out, rq, path, (int)strlen(path));
	out->f->write_body(out, rq, _KS("</a>"));
	if (isdir) {
		out->f->write_body(out, rq, _KS("]"));
	}
	out->f->write_body(out, rq, _KS("</td><td>"));
	f.str("");
	if (isdir) {
		//s << "<td colspan=2>&lt;DIR&gt;</td>");
		f << "-";
	} else {
		f << sbuf.st_size;
	}
	out->f->write_body(out, rq, f.str().c_str(), (int)f.str().size());
	out->f->write_body(out, rq, _KS("</td><td>"));
	char tmp[27];
	make_last_modified_time(&sbuf.st_mtime, tmp, 27);
	out->f->write_body(out, rq, tmp, (int)strlen(tmp));
	return out->f->write_body(out, rq, _KS("</td></tr>\n"));
}
