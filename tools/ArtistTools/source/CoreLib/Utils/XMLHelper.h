#pragma once
#include <QtCore\QString>
#include <QtCore\QList>
#include "corelib_global.h"

class QDomDocument;

class CORELIB_EXPORT XMLFile
{
public:
	XMLFile(const QString& rootName);

	void load(const QString& filePath);
	void save(const QString& filePath);

protected:
	virtual void loadItems(QDomDocument xmlDoc) = 0;
	virtual void saveItems(QDomDocument xmlDoc) = 0;
protected:
	QString		_rootName;
};

class CORELIB_EXPORT SingleItemKindFile : public XMLFile
{
public:
	SingleItemKindFile(const QString& rootName, const QString& itemName);

	virtual void loadItems(QDomDocument xmlDoc);
	virtual void saveItems(QDomDocument xmlDoc);

	void addItemBack(const QString& item) { _items.push_back(item); }
	void addItemFront(const QString& item) { _items.push_back(item); }
	QList<QString>& getItems() { return _items; }
	bool isItemExist(const QString& item);

private:
	QString				_itemName;
	QList<QString>		_items;
};
