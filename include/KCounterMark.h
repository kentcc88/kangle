#ifndef KCOUNTER_MARK_H
#define KCOUNTER_MARK_H
class KCounterMark : public KMark
{
public:
	KCounterMark()
	{
		counter = 0;
	}
	~KCounterMark()
	{
	}
	bool process(KHttpRequest* rq, KHttpObject* obj, KSafeSource& fo) override
	{
		lock.Lock();
		counter++;
		lock.Unlock();
		return true;
	}
	KMark * new_instance() override
	{
		return new KCounterMark();
	}
	const char *getName() override
	{
		return "counter";
	}
	void get_html(KModel* model, KWStream& s) override {
		s << "<input type=checkbox name='reset' value='1'>reset";
	}
	void get_display(KWStream& s) override {
		s << counter;
	}
	void parse_config(const khttpd::KXmlNodeBody* xml) override {
	}
private:
	KMutex lock;
	INT64 counter;
};
#endif
