#include <climits>
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>
#include <QDialog>
#include <QFutureWatcher>
#include <QGraphicsScene>
#include <QMainWindow>
#include <dialogs/AboutDialog.h>

#include "models/CImage.h"
#include "models/CImageTreeModel.h"
#include "utils/Updater.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_actionAbout_Caesium_Image_Compressor_triggered();
    void on_addFiles_Button_clicked();
    void on_actionAdd_files_triggered();
    void on_compress_Button_clicked();
    void on_actionAdd_folder_triggered();
    void on_removeFiles_Button_clicked();
    void on_outputFolderBrowse_Button_clicked();
    void on_outputSuffix_LineEdit_textChanged(const QString& arg1);
    void on_compression_Slider_valueChanged(int value);
    void on_actionRemove_triggered();
    void on_actionClear_triggered();
    void on_resize_groupBox_toggled(bool checked);
    void on_width_SpinBox_valueChanged(int arg1);
    void on_height_SpinBox_valueChanged(int arg1);
    void on_edge_SpinBox_valueChanged(int arg1);
    void on_keepAspectRatio_CheckBox_toggled(bool checked);
    void on_doNotEnlarge_CheckBox_toggled(bool checked);
    void on_actionSelect_All_triggered();
    void on_sameOutputFolderAsInput_CheckBox_toggled(bool checked);
    void on_keepStructure_Checkbox_toggled(bool checked);
    void imageList_selectionChanged(const QModelIndex& current, const QModelIndex& previous);
    void compressionFinished();
    void dropFinished(QStringList filePaths);
    void on_fitTo_ComboBox_currentIndexChanged(int index);
    void on_lossless_Checkbox_toggled(bool checked);
    void on_keepMetadata_Checkbox_toggled(bool checked);
    void on_advancedMode_Button_toggled(bool checked);
    void on_JPEGQuality_Slider_valueChanged(int value);
    void on_JPEGQuality_SpinBox_valueChanged(int arg1);
    void on_PNGIterations_SpinBox_valueChanged(int arg1);
    void on_PNGIterationsLarge_SpinBox_valueChanged(int arg1);
    void on_PNGLossy8_CheckBox_toggled(bool checked);
    void on_PNGLossyTransparent_CheckBox_toggled(bool checked);
    void on_updateAvailable_Button_clicked();
    void cModelItemsChanged();
public slots:
    void updateAvailable(const QString &filePath);
    void runUpdate();

private:
    Ui::MainWindow* ui;

    CImageTreeModel* cImageModel;
    QFutureWatcher<void> compressionWatcher;
    QGraphicsScene* previewScene;
    QMap<QString, int> folderMap;
    QThread updaterThread;
    AboutDialog* aboutDialog = nullptr;
    QVariant readSetting(const QString& key);
    QString updateFilePath;
    QString lastOpenedDirectory;
    QString importedFilesRootFolder;

    void initStatusBar();
    void initUpdater();

    void importFiles(const QStringList& fileList, QString baseFolder);
    void removeFiles(bool all = false);
    void triggerImportFiles();
    void triggerImportFolder();
    void writeSettings();
    void writeSetting(const QString& key, const QVariant& value);
    void readSettings();
    void previewImage(const QModelIndex& imageIndex);
    void updateFolderMap(QString baseFolder, int count);
};

#endif // MAINWINDOW_H
