#ifndef DisplayPreferencesPanel_h__
#define DisplayPreferencesPanel_h__

#include <QtWidgets/QWidget>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include "TipListView.h"
#include "ui_DisplayPreferencesPanel.h"

#include "corelib_global.h"

struct PlaylistParamsContext;

class PlaylistsLoader
{
public:
	PlaylistsLoader();
	~PlaylistsLoader();

	void loadPlaylistsFromMediaPath();
	bool rename(int idx, QString& newName);
	bool remove(int idx);
	bool add(QString& name);
	bool saveProjectsInPlaylist(int idx, QList<QString>& projects);
	int getProjectsInPlaylist(int idx, QList<QString>& projects);

	static QString convertToSaveingFilePath(QString& filePath);
	static QString convertToAbsoluteFilePath(QString& filePath);

	QList<QFileInfo> playlists;
	static QString projectPath;
	static QString mediaPath;

private:
	void CreatePlaylistParamsContext();
	void ReleasePlaylistParamsContext();
	PlaylistParamsContext* pContext;
};

class DisplayPreferencesPanel : public QWidget
{
	Q_OBJECT

public:
	DisplayPreferencesPanel(QWidget* parent);
	~DisplayPreferencesPanel();

	public:
		void updateValues();

	public slots:
	CORELIB_EXPORT void on_spinRenderPlayRateFPS_valueChanged(double v);
	CORELIB_EXPORT void on_spinSimulationRateFPS_valueChanged(double v);
	CORELIB_EXPORT void on_spinCameraFOV_valueChanged(double v);

	CORELIB_EXPORT void on_cbSceneUnit_currentIndexChanged(int index);
	CORELIB_EXPORT void on_cbAntialiasing_currentIndexChanged(int index);
	CORELIB_EXPORT void on_cbUpAxis_currentIndexChanged(int index);
	CORELIB_EXPORT void on_cbHandedness_currentIndexChanged(int index);
	CORELIB_EXPORT void on_cbNavigationStyle_currentIndexChanged(int index);
	CORELIB_EXPORT void on_btnBackgroundTex_clicked();
	CORELIB_EXPORT void on_btnBackgroundTexClear_clicked();

	CORELIB_EXPORT void on_cbPlaylists_currentIndexChanged(int index);
	CORELIB_EXPORT void SelectPlayList(QString& name);
	CORELIB_EXPORT void on_listWidgetPlaylist_itemSelectionChanged();
	CORELIB_EXPORT void on_listWidgetPlaylist_itemDoubleClicked(QListWidgetItem* pItem);
	CORELIB_EXPORT void SelectProject(QString& name);

	CORELIB_EXPORT void on_btnPlaylistsRename_clicked();
	CORELIB_EXPORT void on_btnPlaylistsAdd_clicked();
	CORELIB_EXPORT void on_btnPlaylistsReload_clicked();
	CORELIB_EXPORT void on_btnPlaylistsRemove_clicked();

	CORELIB_EXPORT void on_btnPlaylistAddProj_clicked();
	CORELIB_EXPORT void on_btnPlaylistProjGoUp_clicked();
	CORELIB_EXPORT void on_btnPlaylistProjGoDown_clicked();
	CORELIB_EXPORT void on_btnPlaylistRemoveProj_clicked();

	CORELIB_EXPORT void on_btnPlaylistsPlay_clicked();
	CORELIB_EXPORT void assignPlayPlaylistToGamepad(int idx = -1);
	CORELIB_EXPORT void playPlaylist(int idx);

	CORELIB_EXPORT void reloadComboxBoxPlaylists();
	CORELIB_EXPORT void refreshListboxPorjects();
	CORELIB_EXPORT void reloadListboxPorjectsFromPlayist();
	CORELIB_EXPORT void savePlaylistProjects();

	CORELIB_EXPORT void runDemoCommandline();

private:
	Ui::DisplayPreferencesPanel ui;
	PlaylistsLoader playlistsLoader;
	QList<QString>  playlistProjects;
	bool playlistProjectsDirty;
	int idxCurrentPlaylist;
	int idxCurrentProj;
};

#endif // DisplayScene_h__
