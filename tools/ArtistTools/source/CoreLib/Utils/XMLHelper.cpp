#include "XMLHelper.h"
#include <QtXml\QtXml>
#include "ViewerOutput.h"

static int gcounter = 0;
static char gbuf[256];

XMLFile::XMLFile(const QString& rootName)
	: _rootName(rootName)
{
}

void XMLFile::load(const QString& filePath)
{
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly))
	{
		return;
	}

	QDomDocument xmlDoc;
	if (!xmlDoc.setContent(&file))
	{
		file.close();
		return;
	}
	file.close();

	if (xmlDoc.isNull() || xmlDoc.documentElement().tagName() != _rootName)
	{
		sprintf(gbuf, "The file you selected is empty or not a speficied file: %s.", filePath.toStdString().c_str());
		viewer_msg(gbuf);
		return;
	}

	loadItems(xmlDoc);

	//QDomNodeList elms = xmlDoc.documentElement().elementsByTagName(QObject::tr("StressSolverPreset"));
	//for (int i = 0; i < elms.count(); ++i)
	//{
	//	StressSolverUserPreset preset("");
	//	_userPresets.push_back(preset);
	//	_loadStressSolverPreset(elms.at(i).toElement(), _userPresets[i]);
	//}
}

void XMLFile::save(const QString& filePath)
{
	std::string rr = filePath.toStdString();
	rr;

	QFileInfo fileInfo(filePath);
	QDir fileDir = fileInfo.absoluteDir();

	QString tt = fileDir.absolutePath();
	std::string hh = tt.toStdString();
	hh;

	if (!fileDir.exists())
	{
		if (!fileDir.mkdir(fileDir.absolutePath()))
		{
			sprintf(gbuf, "Failed to crreate the folder: %s.", filePath.toStdString().c_str());
			viewer_msg(gbuf);
			return;
		}
	}
	QFile file(filePath);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		return;
	}
	QTextStream out(&file);

	QDomDocument xmlDoc;
	QDomElement rootElm = xmlDoc.createElement(_rootName);
	xmlDoc.appendChild(rootElm);

	saveItems(xmlDoc);

	// 4 is count of indent
	xmlDoc.save(out, 4);
}

SingleItemKindFile::SingleItemKindFile(const QString& rootName, const QString& itemName)
	: XMLFile(rootName)
	, _itemName(itemName)
{
}

void SingleItemKindFile::loadItems(QDomDocument xmlDoc)
{
	QDomNodeList elms = xmlDoc.documentElement().elementsByTagName(_itemName);
	for (int i = 0; i < elms.count(); ++i)
	{
		QDomElement elm = elms.at(i).toElement();
		_items.push_back(elm.attribute(QObject::tr("Value")));
	}
}

void SingleItemKindFile::saveItems(QDomDocument xmlDoc)
{
	for (int i = 0; i < _items.count(); ++i)
	{
		QDomElement elm = xmlDoc.createElement(_itemName);
		elm.setAttribute(QObject::tr("Value"), _items.at(i));
		xmlDoc.documentElement().appendChild(elm);
	}
}

bool SingleItemKindFile::isItemExist(const QString& item)
{
	for (int i = 0; i < _items.count(); ++i)
	{
		if (_items.at(i) == item)
			return true;
	}

	return false;
}
