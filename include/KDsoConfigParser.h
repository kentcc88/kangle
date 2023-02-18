#ifndef KDSOCONFIGPARSER_H_99
#define KDSOCONFIGPARSER_H_99
#include "KXmlEvent.h"
#include "KConfigTree.h"
class KDsoConfigListen : public kconfig::KConfigListen
{
public:
	void on_event(kconfig::KConfigTree* tree, KXmlNode* xml, kconfig::KConfigEventType ev);
	bool is_merge() {
		return false;
	}
};
#endif
