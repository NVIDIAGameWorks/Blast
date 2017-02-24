#include "DisplayPreferencesPanel.h"

#include "AppMainWindow.h"

#include "SimpleScene.h"
#include "GlobalSettings.h"
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QFileDialog>

#include "PlaylistParams.h"
//#include <Nv/Blast/NvHairCommon.h>
#include "NvParameterized.h"
#include "XmlSerializer.h"
#include "NsFileBuffer.h"
#include "NvTraits.h"
#include "NsMemoryBuffer.h"
#include "ViewerOutput.h"
#include "Settings.h"

#include "Gamepad.h"

const QString sPlaylistExt = "plist";
bool bRefreshingComboBox = false;

DisplayPreferencesPanel::DisplayPreferencesPanel(QWidget* parent)
	:
	QWidget(parent)
	, idxCurrentPlaylist(-1)
	, idxCurrentProj(-1)
	, playlistProjectsDirty(false)
{
	ui.setupUi(this);

	ui.btnBackgroundTex->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));
	ui.btnBackgroundTex->setIconSize(QSize(12,12));
	ui.btnBackgroundTexClear->setIcon(QIcon(":/AppMainWindow/images/Remove_icon.png"));

	ui.btnPlaylistsRename->setIcon(QIcon(":/AppMainWindow/images/EditWrench.png")); 
	ui.btnPlaylistsAdd->setIcon(QIcon(":/AppMainWindow/images/Add.png"));
	ui.btnPlaylistsReload->setIcon(QIcon(":/AppMainWindow/images/refreshReload.png"));
	ui.btnPlaylistsRemove->setIcon(QIcon(":/AppMainWindow/images/Remove_icon.png"));

	ui.btnPlaylistAddProj->setIcon(QIcon(":/AppMainWindow/images/Add.png"));
	ui.btnPlaylistRemoveProj->setIcon(QIcon(":/AppMainWindow/images/Remove_icon.png"));
	ui.btnPlaylistProjGoUp->setIcon(QIcon(":/AppMainWindow/images/Up.png"));
	ui.btnPlaylistProjGoDown->setIcon(QIcon(":/AppMainWindow/images/Down.png"));

	ui.btnPlaylistsPlay->setIcon(QIcon(":/AppMainWindow/images/playlist.png"));

	//QObject::connect(ui.listWidgetPlaylist, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(playlistDoubleClicked(QListWidgetItem*)));

	ConfigSpinBox<SlideSpinBoxF>(ui.spinCameraFOV,	5.0, 180.0, 1.0);
}

DisplayPreferencesPanel::~DisplayPreferencesPanel()
{
}

void DisplayPreferencesPanel::on_spinRenderPlayRateFPS_valueChanged(double v)
{
	GlobalSettings::Inst().setRenderFps((float)v);
}

void DisplayPreferencesPanel::on_spinSimulationRateFPS_valueChanged(double v)
{
	GlobalSettings::Inst().setSimulationFps((float)v);
}

void DisplayPreferencesPanel::on_spinCameraFOV_valueChanged(double v)
{
	GlobalSettings::Inst().m_fovAngle = v;
}

void DisplayPreferencesPanel::on_cbAntialiasing_currentIndexChanged( int index )
{
	GlobalSettings::Inst().m_msaaOption = index;
}

void DisplayPreferencesPanel::on_cbUpAxis_currentIndexChanged( int index )
{
	switch (index)
	{
	case 0: // y up
		SimpleScene::Inst()->ResetUpDir(false);
		break;
	case 1: // z up
		SimpleScene::Inst()->ResetUpDir(true);
		break;
	}
	SimpleScene::Inst()->SetProjectModified(true);
}

void DisplayPreferencesPanel::on_cbHandedness_currentIndexChanged( int index )
{
	switch (index)
	{
	case 0: // rhs
		SimpleScene::Inst()->ResetLhs(false);
		break;
	case 1: // lhs
		SimpleScene::Inst()->ResetLhs(true);
		break;
	}
	SimpleScene::Inst()->SetProjectModified(true);
}

void DisplayPreferencesPanel::on_cbSceneUnit_currentIndexChanged( int index )
{
	GlobalSettings::Inst().m_sceneUnitIndex = index;
}

void DisplayPreferencesPanel::on_cbNavigationStyle_currentIndexChanged( int index )
{
	AppMainWindow::Inst().setNavigationStyle(index);
}

void DisplayPreferencesPanel::on_btnBackgroundTex_clicked()
{
	if (SimpleScene::Inst()->LoadBackgroundTextureFile())
		ui.btnBackgroundTex->setIcon(QIcon(":/AppMainWindow/images/TextureIsUsed_icon.png"));
	else
		ui.btnBackgroundTex->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));

	updateValues();
	SimpleScene::Inst()->SetProjectModified(true);
}

void DisplayPreferencesPanel::on_btnBackgroundTexClear_clicked()
{
	SimpleScene::Inst()->ClearBackgroundTexture();
	ui.btnBackgroundTex->setIcon(QIcon(":/AppMainWindow/images/TextureEnabled_icon.png"));
	updateValues();
}

void DisplayPreferencesPanel::updateValues()
{
	GlobalSettings& globalSettings = GlobalSettings::Inst();

	ui.spinRenderPlayRateFPS->setValue(globalSettings.m_renderFps);
	ui.spinSimulationRateFPS->setValue(globalSettings.m_simulationFps);
	ui.cbUpAxis->setCurrentIndex(globalSettings.m_zup);
	ui.cbHandedness->setCurrentIndex(globalSettings.m_lhs);
	ui.spinCameraFOV->setValue(globalSettings.m_fovAngle);
	ui.cbSceneUnit->setCurrentIndex(globalSettings.m_sceneUnitIndex);
	ui.cbNavigationStyle->setCurrentIndex(AppMainWindow::Inst().getNavigationStyle());
	ui.cbAntialiasing->setCurrentIndex(globalSettings.m_msaaOption);

	savePlaylistProjects();
	reloadComboxBoxPlaylists();

	//// sync playlist to gamepad
	//static bool bOnce = true;
	//if (bOnce)
	//{
	//	bOnce = false;
	//	assignPlayPlaylistToGamepad(0);
	//}
}

void DisplayPreferencesPanel::reloadComboxBoxPlaylists()
{
	playlistsLoader.loadPlaylistsFromMediaPath();
	// set playlist combo box
	//idxCurrentPlaylist = ui.cbPlaylists->currentIndex();
	bRefreshingComboBox = true;
	ui.cbPlaylists->clear();
	int count = playlistsLoader.playlists.size();
	for (int i = 0; i < count; ++i)
	{
		QFileInfo& fi = playlistsLoader.playlists[i];
		std::string fn = fi.fileName().toUtf8().data();
		ui.cbPlaylists->addItem(fi.fileName());
	}
	bRefreshingComboBox = false;
	if (idxCurrentPlaylist > count || idxCurrentPlaylist < 0)
		idxCurrentPlaylist = 0;

	ui.cbPlaylists->setCurrentIndex(idxCurrentPlaylist);

	// refresh project list
	reloadListboxPorjectsFromPlayist();
}

void DisplayPreferencesPanel::runDemoCommandline()
{
	// run demos if command line asks
	static bool once = true;
	if (once)
	{
		once = false;
		std::string demoPlaylist = AppSettings::Inst().GetOptionValue("User/FurDemoPlaylist")->Value.String;
		if (demoPlaylist.size() > 1)
		{
			QString demoList = QString(demoPlaylist.c_str()).toUpper();
			int count = playlistsLoader.playlists.size();
			for (int i = 0; i < count; ++i)
			{
				QFileInfo& fi = playlistsLoader.playlists[i];
				QString fname = fi.fileName().toUpper();
				if (fname == demoList)
				{
					idxCurrentPlaylist = i;
					ui.cbPlaylists->setCurrentIndex(idxCurrentPlaylist);
					playPlaylist(idxCurrentPlaylist);
					return;
				}
			}
			viewer_msg(QString("Cannot find playlist, " + demoList).toUtf8().data());
		}
	}
}

void DisplayPreferencesPanel::reloadListboxPorjectsFromPlayist()
{
	int count = playlistsLoader.getProjectsInPlaylist(idxCurrentPlaylist, playlistProjects);
	refreshListboxPorjects();
}

void DisplayPreferencesPanel::savePlaylistProjects()
{
	if (playlistProjectsDirty)
	{
		playlistsLoader.saveProjectsInPlaylist(idxCurrentPlaylist, playlistProjects);
		playlistProjectsDirty = false;
	}
}

void DisplayPreferencesPanel::refreshListboxPorjects()
{
	int count = playlistProjects.size();
	ui.listWidgetPlaylist->clear();
	ui.listWidgetPlaylist->setSortingEnabled(false);
	ui.listWidgetPlaylist->setSelectionMode(QAbstractItemView::SingleSelection);
	QStringList tips;
	for (int i = 0; i < count; ++i)
	{
		QString fullName = playlistsLoader.convertToAbsoluteFilePath(playlistProjects[i]);
		QFileInfo fi(fullName);
		tips.append(fi.absoluteFilePath());
		std::string tmp = fi.absoluteFilePath().toUtf8().data();
		ui.listWidgetPlaylist->addItem(fi.fileName());
	}
	ui.listWidgetPlaylist->setTips(tips);
	if (idxCurrentProj > count || idxCurrentProj < 0)
	{
		idxCurrentProj = 0;
	}
	ui.listWidgetPlaylist->setCurrentRow(idxCurrentProj);
}

void DisplayPreferencesPanel::SelectPlayList(QString& name)
{
	QString newName = name.toUpper();
	int count = playlistsLoader.playlists.size();
	for (int i = 0; i < count; ++i)
	{
		QFileInfo& fi = playlistsLoader.playlists[i];
		if (fi.baseName().toUpper() == newName)
		{
			ui.cbPlaylists->setCurrentIndex(i);
			break;
		}
	}
}

void DisplayPreferencesPanel::on_cbPlaylists_currentIndexChanged(int index)
{
	savePlaylistProjects();
	if (!bRefreshingComboBox)
	{
		idxCurrentPlaylist = ui.cbPlaylists->currentIndex();
		reloadListboxPorjectsFromPlayist();
	}
}

void DisplayPreferencesPanel::SelectProject(QString& name)
{
	QString newName = name.toUpper();
	std::string strNew = newName.toUtf8().data();
	int count = playlistProjects.size();
	for (int i = 0; i < count; ++i)
	{
		QString& str = playlistProjects[i];
		//QFileInfo fi(str);
		//std::string stmp = fi.absoluteFilePath().toUtf8().data();
		//if (fi.absoluteFilePath().toUpper() == newName)
		if (str.toUpper() == newName)
		{
			ui.listWidgetPlaylist->setCurrentRow(i);
			break;
		}
	}
}

void DisplayPreferencesPanel::on_listWidgetPlaylist_itemSelectionChanged()
{
	idxCurrentProj = ui.listWidgetPlaylist->currentRow();
}

void DisplayPreferencesPanel::on_listWidgetPlaylist_itemDoubleClicked(QListWidgetItem* pItem)
{
	idxCurrentProj = ui.listWidgetPlaylist->currentRow();
	if (idxCurrentProj < 0)
		return;
	QString fn = playlistProjects[idxCurrentProj];
	QString fullname = PlaylistsLoader::convertToAbsoluteFilePath(fn);
	AppMainWindow::Inst().openProject(fullname);
	char msg[1024];
	sprintf(msg, "Blast Viewer - %s", fullname.toUtf8().data());
	AppMainWindow::Inst().setWindowTitle(msg);
}

void DisplayPreferencesPanel::on_btnPlaylistsRename_clicked()
{
	idxCurrentPlaylist = ui.cbPlaylists->currentIndex();
	if (idxCurrentPlaylist < 0)
		return;

	QFileInfo& fi = playlistsLoader.playlists[idxCurrentPlaylist];
	bool isOK = false;
	QString newBaseName = QInputDialog::getText(NULL, "Rename Playlist - " + fi.fileName(),
		"Input a new name for the playlist, " + fi.fileName(), QLineEdit::Normal,
		fi.baseName(), &isOK);
	if (isOK)
	{
		bool ok = playlistsLoader.rename(idxCurrentPlaylist, newBaseName);
		if (ok)
		{
			reloadComboxBoxPlaylists();
			SelectPlayList(newBaseName);
		}
		else
		{
			QMessageBox::warning(this, "Fail to rename a playlist",
				"Fail to rename a playlist. The failure could relate to name duplication or hardware failure.",
				QMessageBox::Close);
		}
	}
}

void DisplayPreferencesPanel::on_btnPlaylistsAdd_clicked()
{
	savePlaylistProjects();

	bool isOK = false;
	QString sInput = QInputDialog::getText(NULL, "Create New Playlist", "Input a name for the new playlist.", QLineEdit::Normal, "", &isOK);
	if (isOK)
	{
		QString newName = sInput.trimmed();
		bool ok = playlistsLoader.add(newName);
		if (ok)
		{
			reloadComboxBoxPlaylists();
			// select the new one
			SelectPlayList(newName);
		}
		else
		{
			QMessageBox::warning(this, "Fail to add new playlist",
				"Fail to add new playlist. The failure could relate to name duplication or hardware failure.",
				QMessageBox::Close);
		}
	}
}

void DisplayPreferencesPanel::on_btnPlaylistsReload_clicked()
{
	savePlaylistProjects();
	reloadComboxBoxPlaylists();
}

void DisplayPreferencesPanel::on_btnPlaylistsRemove_clicked()
{
	savePlaylistProjects();
	idxCurrentPlaylist = ui.cbPlaylists->currentIndex();
	if (idxCurrentPlaylist < 0)
		return;

	bool ok = playlistsLoader.remove(idxCurrentPlaylist);
	if (ok)
	{
		reloadComboxBoxPlaylists();
	}
	else
	{
		QMessageBox::warning(this, "Fail to remove a playlist",
			"Fail to remove a playlist. The failure could relate to hardware.",
			QMessageBox::Close);
	}
}

void DisplayPreferencesPanel::on_btnPlaylistAddProj_clicked()
{
	static QString lastPath;
	if (lastPath.size() < 1)
	{
		lastPath = PlaylistsLoader::mediaPath;
	}
	QString fileNameInput = QFileDialog::getOpenFileName(this, "Open Hair Project File", lastPath, "Hair Project File (*.furproj)");
	if (!QFile::exists(fileNameInput))
		return;
	std::string tmp = fileNameInput.toUtf8().data();
	QString fileName = PlaylistsLoader::convertToSaveingFilePath(fileNameInput);
	if (fileName.size() < 1)
	{
		return;
	}
	lastPath = QFileInfo(fileNameInput).absolutePath();
	std::string tmp2 = fileName.toUtf8().data();
	playlistProjects.append(fileName);
	refreshListboxPorjects();
	//QFileInfo fi(fileName);
	SelectProject(fileName);// fi.absoluteFilePath());
	playlistProjectsDirty = true;
}

void DisplayPreferencesPanel::on_btnPlaylistProjGoUp_clicked()
{
	idxCurrentProj = ui.listWidgetPlaylist->currentRow();
	if (idxCurrentProj <= 0)
		return;

	QString tmp = playlistProjects[idxCurrentProj];
	playlistProjects[idxCurrentProj] = playlistProjects[idxCurrentProj - 1];
	playlistProjects[idxCurrentProj - 1] = tmp;

	refreshListboxPorjects();
	SelectProject(tmp);

	playlistProjectsDirty = true;
}

void DisplayPreferencesPanel::on_btnPlaylistProjGoDown_clicked()
{
	int count = playlistProjects.size();
	idxCurrentProj = ui.listWidgetPlaylist->currentRow();
	if (idxCurrentProj < 0 || idxCurrentProj>= count -1)
		return;

	QString tmp = playlistProjects[idxCurrentProj];
	playlistProjects[idxCurrentProj] = playlistProjects[idxCurrentProj + 1];
	playlistProjects[idxCurrentProj + 1] = tmp;

	refreshListboxPorjects();
	SelectProject(tmp);

	playlistProjectsDirty = true;
}

void DisplayPreferencesPanel::on_btnPlaylistRemoveProj_clicked()
{
	int count = playlistProjects.size();
	idxCurrentProj = ui.listWidgetPlaylist->currentRow();
	if (idxCurrentProj < 0 || idxCurrentProj >= count)
	{
		return;
	}

	playlistProjects.removeAt(idxCurrentProj);

	refreshListboxPorjects();
	playlistProjectsDirty = true;
}

void DisplayPreferencesPanel::assignPlayPlaylistToGamepad(int idx)
{
	if (idx == -1)
		idx = idxCurrentPlaylist;
	int count = playlistsLoader.getProjectsInPlaylist(idx, playlistProjects);
	if (count < 1)
	{
		viewer_msg("Playlist is empty.");
		return;
	}
	QList<QString> files;
	for (int i = 0; i < count; ++i)
	{
		QString fname = PlaylistsLoader::convertToAbsoluteFilePath(playlistProjects[i]);
		if (fname.size() > 0)
		{
			files.append(fname);
		}
	}

	Gamepad& theGamepad = Gamepad::Instance();
	theGamepad.SetDemoProjects(files);
}

void DisplayPreferencesPanel::playPlaylist(int idx)
{
	assignPlayPlaylistToGamepad(idx);
	Gamepad& theGamepad = Gamepad::Instance();
	theGamepad.SetDemoMode(true);
}

void DisplayPreferencesPanel::on_btnPlaylistsPlay_clicked()
{
	savePlaylistProjects();

	idxCurrentPlaylist = ui.cbPlaylists->currentIndex();
	if (idxCurrentPlaylist < 0)
	{
		viewer_msg("No available playlists.");
		return;
	}
	playPlaylist(idxCurrentPlaylist);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace nvidia; 
using namespace nvidia::parameterized;

QString PlaylistsLoader::projectPath;
QString PlaylistsLoader::mediaPath;

#include "FoundationHolder.h"

struct PlaylistParamsContext
{
	//nvidia::NvFoundation* mFoundation;
	NvParameterized::Traits* mTraits;
	PlaylistParamsFactory* mPlaylistParamsFactory;
};

PlaylistsLoader::PlaylistsLoader()
	:pContext(nullptr)
{
}

PlaylistsLoader::~PlaylistsLoader()
{
}

void PlaylistsLoader::CreatePlaylistParamsContext()
{
	if (pContext)
		return;

	PlaylistParamsContext* context = new PlaylistParamsContext;
	//context->mFoundation = FoundationHolder::GetFoundation();
	FoundationHolder::GetFoundation();
	//assert(context->mFoundation != NV_NULL);
	
	context->mTraits = NvParameterized::createTraits();
	context->mPlaylistParamsFactory = new PlaylistParamsFactory;
	context->mTraits->registerFactory(*context->mPlaylistParamsFactory);
	pContext = context;
}

void PlaylistsLoader::ReleasePlaylistParamsContext()
{
	if (pContext)
	{
		pContext->mTraits->release();
		delete pContext->mPlaylistParamsFactory;
		delete pContext;
		pContext = nullptr;
	}
}

void PlaylistsLoader::loadPlaylistsFromMediaPath()
{
	if (projectPath.isEmpty())
	{
		QString appDir = qApp->applicationDirPath();
		QDir dirTmp(appDir);
		if (dirTmp.cd("./media/playlists"))
			projectPath = dirTmp.absolutePath();
		else if (dirTmp.cd("../media/playlists"))
			projectPath = dirTmp.absolutePath();
		else if (dirTmp.cd("../../media/playlists"))
			projectPath = dirTmp.absolutePath();
		else if (dirTmp.cd("../../media"))
		{
			bool ok = dirTmp.mkdir("playlists");
			if (dirTmp.cd("playlists"))
			{
				projectPath = dirTmp.absolutePath();
			}
		}
		if (!projectPath.isEmpty())
		{
			if (dirTmp.cd(".."))
			{
				mediaPath = dirTmp.absolutePath();
			}
			else
			{
				mediaPath = projectPath + "/..";
			}
		}
	}
	if (!projectPath.isEmpty())
	{
		playlists.clear();

		QStringList filters;
		filters << (QString("*.") + sPlaylistExt);
		QDirIterator dir_iterator(projectPath, filters, QDir::Files | QDir::NoSymLinks, QDirIterator::NoIteratorFlags);  //QDirIterator::Subdirectories
		while (dir_iterator.hasNext())
		{
			dir_iterator.next();
			QFileInfo file_info = dir_iterator.fileInfo();
			std::string absolute_file_path = file_info.absoluteFilePath().toUtf8().data();
			playlists.append(file_info);
		}
	}
}

bool PlaylistsLoader::rename(int idx, QString& newBaseName)
{
	int count = playlists.size();
	if (idx >= 0 && idx < count)
	{
		QFileInfo& finfo = playlists[idx];
		QString fn = finfo.baseName().toUpper();
		QString fnNew = newBaseName.toUpper();
		if (fn != fnNew)
		{
			QFileInfo nfi(projectPath, newBaseName + "." + sPlaylistExt);
			QString fullNewName = nfi.absoluteFilePath();
			std::string fntmp = fullNewName.toUtf8().data();
			bool exist = QFile::exists(fullNewName);
			if (exist)
			{
				return false;
			}
			bool ok = QFile::rename(finfo.absoluteFilePath(), fullNewName);
			if (ok)
			{
				finfo.setFile(fullNewName);
				exist = QFile::exists(fullNewName);
				if (!exist)
				{
					return false;
				}
			}
			return ok;
		}
		return true;
	}
	return false;
}

bool PlaylistsLoader::remove(int idx)
{
	int count = playlists.size();
	if (idx >= 0 && idx < count)
	{
		QFileInfo& finfo = playlists[idx];
		std::string fn = finfo.fileName().toUtf8().data();
		bool ok = QFile::remove(finfo.absoluteFilePath());
		if (ok)
		{
			playlists.removeAt(idx);
		}
		return ok;
	}
	return false;
}

bool PlaylistsLoader::saveProjectsInPlaylist(int idx, QList<QString>& projects)
{ 
	int count = playlists.size();
	if (idx < 0 || idx >= count)
		return false;

	CreatePlaylistParamsContext();
	if (!pContext)
		return false;

	std::string tempFilePath = playlists[idx].absoluteFilePath().toUtf8().data();
	NvParameterized::XmlSerializer serializer(pContext->mTraits);
	NvFileBuf* stream = new NvFileBufferBase(tempFilePath.c_str(), NvFileBuf::OPEN_WRITE_ONLY);
	if (!stream || !stream->isOpen())
	{
		// file open error
		if (stream) stream->release();
		return false;
	}
	NvParameterized::Traits* traits = pContext->mTraits;
	int numObjects = 0;
	const int kMaxObjects = 1;
	NvParameterized::Interface* objects[kMaxObjects];

	PlaylistParams* params = new PlaylistParams(traits);
	objects[numObjects++] = params;
	NvParameterized::Interface* iface = static_cast<NvParameterized::Interface*>(params);

	if (1)
	{
		nvidia::parameterized::PlaylistParams* params = static_cast<nvidia::parameterized::PlaylistParams*>(iface);
		nvidia::parameterized::PlaylistParamsNS::ParametersStruct& targetDesc = params->parameters();

		NvParameterized::Handle handle(iface);
		if (iface->getParameterHandle("furprojFilePaths", handle) == NvParameterized::ERROR_NONE)
		{
			int num = projects.size();

			std::vector<std::string> strArray;
			const char** strOutput = new const char*[num];

			for (int i = 0; i < num; i++)
			{
				std::string proj = projects[i].toUtf8().data();
				strArray.push_back(proj);
				strOutput[i] = strArray[i].c_str();
			}

			handle.resizeArray(num);
			handle.setParamStringArray(strOutput, num);

			delete[] strOutput;
		}
	}

	NV_ASSERT(numObjects <= kMaxObjects);
	NvParameterized::Serializer::ErrorType serError = NvParameterized::Serializer::ERROR_NONE;
	bool isUpdate = false;
	serError = serializer.serialize(*stream, (const NvParameterized::Interface**)&objects[0], numObjects, isUpdate);
	for (int idx = 0; idx < numObjects; ++idx)
	{
		delete objects[idx];
	}
	stream->release();
	ReleasePlaylistParamsContext();
	return true;
}

QString PlaylistsLoader::convertToAbsoluteFilePath(QString& filePath)
{
	QString fname;
	bool bCanBeRelativePath = false;
	bool bAbsPath = (filePath.indexOf(':') >= 0);
	if (bAbsPath)
	{
		if (QFile::exists(filePath))
		{
			fname = filePath;
		}
		else
		{
			viewer_msg(QString(filePath + " does not exist.").toUtf8().data());
		}
		std::string tmp2 = fname.toUtf8().data();
		return fname;
	}
	else
	{
		QFileInfo fi(mediaPath + "/" + filePath);
		fname = fi.absoluteFilePath();
		if (!QFile::exists(fname))
		{
			viewer_msg(QString(filePath + " does not exist.").toUtf8().data());
			fname = "";
		}
		std::string tmp2 = fname.toUtf8().data();
		return fname;
	}
}

QString PlaylistsLoader::convertToSaveingFilePath(QString& filePath)
{
	QString fname;
	bool bCanBeRelativePath = false;
	bool bAbsPath = (filePath.indexOf(':') >= 0);
	if (bAbsPath)
	{
		if (QFile::exists(filePath))
		{
			QFileInfo fi(filePath);
			int pos = fi.absoluteFilePath().indexOf(mediaPath, Qt::CaseInsensitive);
			if (pos >= 0)
			{
				// convert to relative path
				fname = filePath.right(filePath.size() - (pos + mediaPath.size() + 1));
			}
			else
			{
				fname = fi.absoluteFilePath();
			}
		}
		else
		{
			fname = "";
			viewer_msg(QString(filePath + " does not exist.").toUtf8().data());
		}
		std::string tmp = fname.toUtf8().data();
		return fname;
	}
	// do more try
	QString tag = "media";
	int pos = filePath.indexOf(tag, Qt::CaseInsensitive);
	if (pos >= 0)
	{
		fname = filePath.right(filePath.size() - (pos + tag.size() + 1));
	}
	else
	{
		fname = filePath;
	}
	QFileInfo fi(mediaPath + "/" + fname);
	std::string tmp = fi.absoluteFilePath().toUtf8().data();
	if (!QFile::exists(fi.absoluteFilePath()))
	{
		viewer_msg(QString(filePath + " does not exist.").toUtf8().data());
		fname = "";
	}
	std::string tmp2 = fname.toUtf8().data();
	return fname;
}

int PlaylistsLoader::getProjectsInPlaylist(int idx, QList<QString>& projects)
{
	projects.clear();

	int count = playlists.size();
	if (idx < 0 || idx >= count)
		return 0;

	CreatePlaylistParamsContext();
	if (!pContext)
		return 0;

	std::string tempFilePath = playlists[idx].absoluteFilePath().toUtf8().data();
	NvFileBuf* stream = new NvFileBufferBase(tempFilePath.c_str(), NvFileBuf::OPEN_READ_ONLY);
	if (!stream || !stream->isOpen())
	{
		// file open error
		if (stream)
			stream->release();
		return 0;
	}

	NvParameterized::Serializer::DeserializedData data;
	NvParameterized::Serializer::ErrorType serError = NvParameterized::Serializer::ERROR_NONE;
	NvParameterized::XmlSerializer serializer(pContext->mTraits);
	bool isUpdated = false;
	serError = serializer.deserialize(*stream, data, isUpdated);
	if (data.size() < 1)
	{
		if (stream)
			stream->release();
		return 0;
	}

	for (int idx = 0; idx < (int)data.size(); ++idx)
	{
		NvParameterized::Interface* iface = data[idx];
		if (::strcmp(iface->className(), PlaylistParams::staticClassName()) == 0)
		{
			nvidia::parameterized::PlaylistParams* params = static_cast<nvidia::parameterized::PlaylistParams*>(iface);
			nvidia::parameterized::PlaylistParamsNS::ParametersStruct& srcDesc = params->parameters();
			NvParameterized::Handle handle(iface);
			if (iface->getParameterHandle("furprojFilePaths", handle) == NvParameterized::ERROR_NONE)
			{
				int arraySize;
				handle.getArraySize(arraySize);
				char** strArray = new char*[arraySize];
				handle.getParamStringArray(strArray, arraySize);
				for (int idx = 0; idx < arraySize; ++idx)
				{
					QString fileName = PlaylistsLoader::convertToSaveingFilePath(QString(strArray[idx]));
					if (fileName.size() > 0)
					{
						std::string tmp = fileName.toUtf8().data();
						projects.append(fileName);
					}
				}
				delete[] strArray;
			}
		}
	}
	stream->release();
	ReleasePlaylistParamsContext();
	return projects.size();
}

bool PlaylistsLoader::add(QString& name)
{
	int count = playlists.size();
	for (int i = 0; i < count; ++i)
	{
		QFileInfo& fi = playlists[i];
		if (fi.baseName().toUpper() == name.toUpper())
		{
			return false;
		}
	}
	QFileInfo nfi(projectPath, name + "." + sPlaylistExt);
	QFile newFile(nfi.absoluteFilePath());
	if (newFile.exists())
	{
		return false;
	}
	newFile.open(QIODevice::WriteOnly);
	newFile.close();
	bool exist = QFile::exists(nfi.absoluteFilePath());
	if (!exist)
	{
		return false;
	}
	playlists.append(nfi);
	return true;
}