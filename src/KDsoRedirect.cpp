#include "KDsoRedirect.h"
#include "KDsoAsyncFetchObject.h"

KRedirectSource*KDsoRedirect::makeFetchObject(KHttpRequest *rq, KFileName *file)
{
	return new KDsoAsyncFetchObject(NULL);
}
KRedirectSource *KDsoRedirect::makeFetchObject(KHttpRequest *rq, void *model_ctx)
{
	return new KDsoAsyncFetchObject(model_ctx);
}