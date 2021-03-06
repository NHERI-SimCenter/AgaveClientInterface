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
// Written by Peter Sempolinski, for the Natural Hazard Modeling Laboratory, director: Ahsan Kareem, at Notre Dame

#include "remotefiletree.h"

#include "filestandarditem.h"
#include "fileoperator.h"
#include "filenoderef.h"

RemoteFileTree::RemoteFileTree(QWidget *parent) :
    QTreeView(parent)
{
    QObject::connect(this, SIGNAL(expanded(QModelIndex)), this, SLOT(folderExpanded(QModelIndex)));
    QObject::connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(fileEntryTouched(QModelIndex)));
    this->setEditTriggers(QTreeView::NoEditTriggers);
}

FileNodeRef RemoteFileTree::getSelectedFile()
{
    QModelIndexList indexList = this->selectedIndexes();

    if (indexList.isEmpty()) return FileNodeRef::nil();
    QStandardItemModel * theModel = qobject_cast<QStandardItemModel *>(this->model());
    if (theModel == nullptr) return FileNodeRef::nil();

    QStandardItem * theItem = theModel->itemFromIndex(indexList.at(0));

    FileStandardItem * retNode = dynamic_cast<FileStandardItem *>(theItem);
    if (retNode == nullptr) return FileNodeRef::nil();
    return retNode->getFile();
}

void RemoteFileTree::selectRowByFile(FileNodeRef toSelect)
{
    if (myOperator == nullptr) return;
    if (toSelect.getFullPath() == getSelectedFile().getFullPath()) return;

    QPersistentModelIndex nodeModelRow = toSelect.getFirstModelIndex();
    if (!nodeModelRow.isValid())
    {
        clearSelection();
        return;
    }

    selectRowByIndex(nodeModelRow);
}

void RemoteFileTree::linkToFileOperator(FileOperator * theOperator)
{
    if (myOperator != nullptr)
    {
        myOperator->disconnectFileTreeWidget(this);
    }
    myOperator = theOperator;
    if (myOperator != nullptr)
    {
        myOperator->connectFileTreeWidget(this);
        header()->resizeSection(0,350);
        header()->resizeSection(1,40);
    }
    emit newFileSelected(FileNodeRef::nil());
}

void RemoteFileTree::folderExpanded(QModelIndex fileIndex)
{
    fileEntryTouched(fileIndex);
    FileNodeRef selectedItem = getSelectedFile();
    if (selectedItem.isNil()) return;
    if (selectedItem.folderContentsLoaded()) return;

    selectedItem.enactFolderRefresh();
}

void RemoteFileTree::fileEntryTouched(QModelIndex itemTouched)
{
    this->selectionModel()->clearSelection();

    if (!itemTouched.isValid())
    {
        return;
    }

    selectRowByIndex(itemTouched);
}

void RemoteFileTree::forceSelectionRefresh()
{
    emit newFileSelected(getSelectedFile());
}

void RemoteFileTree::selectRowByIndex(QModelIndex clickedNode)
{
    QModelIndex firstNode = clickedNode.sibling(clickedNode.row(),0);
    QModelIndex lastNode = clickedNode.sibling(clickedNode.row(),model()->columnCount() - 1);

    this->selectionModel()->select(QItemSelection(firstNode, lastNode), QItemSelectionModel::Select);
    emit newFileSelected(getSelectedFile());
}
