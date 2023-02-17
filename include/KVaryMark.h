#ifndef KVARYMARK_H
#define KVARYMARK_H
class KVaryMark : public KMark
{
public:
	KVaryMark()
	{
		header = NULL;
	}
	~KVaryMark()
	{
		if (header) {
			xfree(header);
		}
	}
	bool mark(KHttpRequest* rq, KHttpObject* obj, KFetchObject** fo) override {
		if (header == NULL) {
			return false;
		}
		kassert(obj);
		if (KBIT_TEST(obj->index.flags, OBJ_HAS_VARY)) {
			return false;
		}
		if (!obj->AddVary(rq, header, header_len)) {
			obj->insert_http_header(kgl_header_vary, header, header_len);
		}
		return true;
	}
	std::string getDisplay()override {
		std::stringstream s;
		if (header) {
			s << header;
		}
		return s.str();
	}
	void editHtml(std::map<std::string, std::string>& attribute, bool html) override {
		if (header) {
			xfree(header);
		}
		header = strdup(attribute["header"].c_str());
		header_len = strlen(header);
	}
	std::string getHtml(KModel* model) override {
		KVaryMark* m = (KVaryMark*)model;
		std::stringstream s;
		s << "header:<input name='header' value='";
		if (m && m->header) {
			s << m->header;
		}
		s << "'>";
		return s.str();
	}
	KMark* new_instance() override {
		return new KVaryMark;
	}
	const char* getName() override {
		return "vary";
	}
	void buildXML(std::stringstream& s) override {
		if (header) {
			s << " header='" << header << "' ";
		}
		s << ">";
	}
private:
	char* header;
	int header_len;
};
#endif
