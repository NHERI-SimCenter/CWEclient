/*********************************************************************************
**
** Copyright (c) 2017 The University of Notre Dame
** Copyright (c) 2017 The Regents of the University of California
**
** Redistribution and use in source and binary forms, with or without modification,
** are permitted provided that the following conditions are met:
**
** 1. Redistributions of source code must retain the above copyright notice, this 
** list of conditions and the following disclaimer.
**
** 2. Redistributions in binary form must reproduce the above copyright notice, this
** list of conditions and the following disclaimer in the documentation and/or other
** materials provided with the distribution.
**
** 3. Neither the name of the copyright holder nor the names of its contributors may
** be used to endorse or promote products derived from this software without specific
** prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
** EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
** SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
** TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
** BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
** CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
** IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
** SUCH DAMAGE.
**
***********************************************************************************/

// Contributors:

#include "cwe_file_manager.h"
#include "ui_cwe_file_manager.h"

#include "../AgaveClientInterface/filemetadata.h"

#include "../AgaveExplorer/remoteFileOps/filetreenode.h"
#include "../AgaveExplorer/remoteFileOps/fileoperator.h"
#include "../AgaveExplorer/utilFuncs/singlelinedialog.h"

#include "vwtinterfacedriver.h"

#include <QFileDialog>

CWE_file_manager::CWE_file_manager(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CWE_file_manager)
{
    ui->setupUi(this);

    // Creates our new model and populate
    localFileModel = new QFileSystemModel(this);

    // Set filter
    localFileModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);
    ui->localTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // Attach the model to the view
    ui->localTreeView->setModel(localFileModel);

    // QFileSystemModel requires root path
    localFileModel->setRootPath(QDir::homePath());
    ui->localTreeView->setRootIndex(localFileModel->setRootPath(QDir::homePath()));
}

CWE_file_manager::~CWE_file_manager()
{
    delete ui;
}

void CWE_file_manager::linkDriver(VWTinterfaceDriver * theDriver)
{
    myDriver = theDriver;
    ui->remoteTreeView->setFileOperator(myDriver->getFileHandler());
    ui->remoteTreeView->setupFileView();
    QObject::connect(ui->remoteTreeView, SIGNAL(customContextMenuRequested(QPoint)),
                         this, SLOT(customFileMenu(QPoint)));
}

void CWE_file_manager::on_pb_upload_clicked()
{
    if (myDriver->getFileHandler()->operationIsPending())
    {
        myDriver->displayMessagePopup("Currently running file operation. Please Wait.");
        return;
    }

    FileTreeNode * targetFile = ui->remoteTreeView->getSelectedNode();

    if ((targetFile == NULL) || (targetFile->getFileData().getFileType() != FileType::DIR))
    {
        myDriver->displayMessagePopup("Please select a destination folder to upload to.");
        return;
    }

    QModelIndex localSelectIndex = ui->localTreeView->currentIndex();
    QFileInfo fileData = localFileModel->fileInfo(localSelectIndex);

    if (!fileData.isFile())
    {
        myDriver->displayMessagePopup("Please select exactly 1 local file to upload.");
        return;
    }

    myDriver->getFileHandler()->sendUploadReq(targetFile, fileData.absoluteFilePath());

    if (!myDriver->getFileHandler()->operationIsPending())
    {
        myDriver->displayMessagePopup("Error: Unable to start file operation. Please try again.");
    }
}

void CWE_file_manager::on_pb_download_clicked()
{
    if (myDriver->getFileHandler()->operationIsPending())
    {
        myDriver->displayMessagePopup("Currently running file operation. Please Wait.");
        return;
    }

    FileTreeNode * targetFile = ui->remoteTreeView->getSelectedNode();

    if ((targetFile == NULL) || (targetFile->getFileData().getFileType() != FileType::FILE))
    {
        myDriver->displayMessagePopup("Please select a file to download to.");
        return;
    }

    QModelIndex localSelectIndex = ui->localTreeView->currentIndex();
    QFileInfo fileData = localFileModel->fileInfo(localSelectIndex);

    if (!fileData.isDir())
    {
        myDriver->displayMessagePopup("Please select exactly 1 local folder to download to");
        return;
    }

    QString localPath = fileData.absoluteFilePath();
#ifdef Q_OS_WIN
    localPath = localPath.append('\\');
#else
    localPath = localPath.append('/');
#endif

    localPath = localPath.append(targetFile->getFileData().getFileName());

    myDriver->getFileHandler()->sendDownloadReq(targetFile, localPath);

    if (!myDriver->getFileHandler()->operationIsPending())
    {
        myDriver->displayMessagePopup("Error: Unable to start file operation. Please try again.");
    }
}

void CWE_file_manager::copyMenuItem()
{
    SingleLineDialog newNamePopup("Please type a file name to copy to:", "newname");
    if (newNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }

    ui->remoteTreeView->getFileOperator()->sendCopyReq(targetNode, newNamePopup.getInputText());
}

void CWE_file_manager::moveMenuItem()
{
    SingleLineDialog newNamePopup("Please type a file name to move to:", "newname");

    if (newNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }

    ui->remoteTreeView->getFileOperator()->sendMoveReq(targetNode,newNamePopup.getInputText());
}

void CWE_file_manager::renameMenuItem()
{
    SingleLineDialog newNamePopup("Please type a new file name:", "newname");

    if (newNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }

    ui->remoteTreeView->getFileOperator()->sendRenameReq(targetNode, newNamePopup.getInputText());
}

void CWE_file_manager::deleteMenuItem()
{
    if (ui->remoteTreeView->getFileOperator()->deletePopup(targetNode))
    {
        ui->remoteTreeView->getFileOperator()->sendDeleteReq(targetNode);
    }
}

void CWE_file_manager::createFolderMenuItem()
{
    SingleLineDialog newFolderNamePopup("Please input a name for the new folder:", "newFolder1");

    if (newFolderNamePopup.exec() != QDialog::Accepted)
    {
        return;
    }
    ui->remoteTreeView->getFileOperator()->sendCreateFolderReq(targetNode, newFolderNamePopup.getInputText());
}

void CWE_file_manager::compressMenuItem()
{
    ui->remoteTreeView->getFileOperator()->sendCompressReq(targetNode);
}

void CWE_file_manager::decompressMenuItem()
{
    ui->remoteTreeView->getFileOperator()->sendDecompressReq(targetNode);
}

void CWE_file_manager::refreshMenuItem()
{
    ui->remoteTreeView->getFileOperator()->enactFolderRefresh(targetNode);
}


void CWE_file_manager::on_remoteTreeView_customContextMenuRequested(const QPoint &pos)
{
    QMenu fileMenu;
    if (ui->remoteTreeView->getFileOperator()->operationIsPending())
    {
        fileMenu.addAction("File Operation In Progress . . .");
        fileMenu.exec(QCursor::pos());
        return;
    }

    QModelIndex targetIndex = ui->remoteTreeView->indexAt(pos);
    ui->remoteTreeView->fileEntryTouched(targetIndex);

    targetNode = ui->remoteTreeView->getSelectedNode();

    //If we did not click anything, we should return
    if (targetNode == NULL) return;
    if (targetNode->isRootNode()) return;
    FileMetaData theFileData = targetNode->getFileData();

    if (theFileData.getFileType() == FileType::INVALID) return;
    if (theFileData.getFileType() == FileType::UNLOADED) return;
    if (theFileData.getFileType() == FileType::EMPTY_FOLDER) return;

    fileMenu.addAction("Copy To . . .",this, SLOT(copyMenuItem()));
    fileMenu.addAction("Move To . . .",this, SLOT(moveMenuItem()));
    fileMenu.addAction("Rename",this, SLOT(renameMenuItem()));
    //We don't let the user delete the username folder
    if (!(targetNode->getParentNode()->isRootNode()))
    {
        fileMenu.addSeparator();
        fileMenu.addAction("Delete",this, SLOT(deleteMenuItem()));
        fileMenu.addSeparator();
    }
    if (theFileData.getFileType() == FileType::DIR)
    {
        fileMenu.addAction("Create New Folder",this, SLOT(createFolderMenuItem()));
    }
    if (theFileData.getFileType() == FileType::FILE)
    {
        fileMenu.addAction("Download File",this, SLOT(downloadMenuItem()));
    }

    if ((theFileData.getFileType() == FileType::DIR) || (theFileData.getFileType() == FileType::FILE))
    {
        fileMenu.addSeparator();
        fileMenu.addAction("Refresh Data",this, SLOT(refreshMenuItem()));
        fileMenu.addSeparator();
    }

    fileMenu.exec(QCursor::pos());
}
