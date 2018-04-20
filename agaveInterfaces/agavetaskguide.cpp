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

#include "agavetaskguide.h"

#include "agavehandler.h"

AgaveTaskGuide::AgaveTaskGuide()
{
    taskId = "INVALID";
}

AgaveTaskGuide::AgaveTaskGuide(QString newID, AgaveRequestType reqType)
{
    taskId = newID;
    requestType = reqType;
}

QString AgaveTaskGuide::getTaskID()
{
    return taskId;
}

QByteArray AgaveTaskGuide::getURLsuffix()
{
    return URLsuffix.toLatin1();
}

QByteArray AgaveTaskGuide::getArgAndURLsuffix(QMap<QString, QByteArray> * varList)
{
    QByteArray ret = getURLsuffix();
    ret.append(fillURLArgList(varList));
    return ret;
}

AgaveRequestType AgaveTaskGuide::getRequestType()
{
    return requestType;
}

AuthHeaderType AgaveTaskGuide::getHeaderType()
{
    return headerType;
}

bool AgaveTaskGuide::isTokenFormat()
{
    return usesTokenFormat;
}

void AgaveTaskGuide::setAsInternal()
{
    internalTask = true;
}

bool AgaveTaskGuide::isInternal()
{
    return internalTask;
}

QByteArray AgaveTaskGuide::fillPostArgList(QMap<QString, QByteArray> *argList)
{
    return fillAnyArgList(argList, &postVarNames, &postFormat);
}

QByteArray AgaveTaskGuide::fillURLArgList(QMap<QString, QByteArray> *argList)
{
    //TODO: Check escaping of other chars
    for (auto itr = argList->begin(); itr != argList->end(); itr++)
    {
        if (!(*itr).contains('#')) continue;
        (*itr).replace('#', "%23");
    }

    return fillAnyArgList(argList, &urlVarNames, &dynURLFormat);
}

QByteArray AgaveTaskGuide::fillAnyArgList(QMap<QString, QByteArray> * argList, QList<QString> * subNames, QString * strFormat)
{
    QByteArray empty;
    if (strFormat == NULL)
    {
        return empty;
    }

    if ((argList == NULL) || (subNames == NULL) || (subNames->empty()))
    {
        return strFormat->toLatin1();
    }

    QString ret = *strFormat;

    for (auto itr = subNames->cbegin(); itr != subNames->cend(); ++itr)
    {
        if (!argList->contains(*itr))
        {
            return empty;
        }

        ret = ret.arg(QString::fromLatin1(argList->value(*itr)));
    }
    return ret.toLatin1();
}

void AgaveTaskGuide::setURLsuffix(QString newValue)
{
    URLsuffix = newValue;
}

void AgaveTaskGuide::setHeaderType(AuthHeaderType newValue)
{
    headerType = newValue;
}

void AgaveTaskGuide::setTokenFormat(bool newSetting)
{
    usesTokenFormat = newSetting;
}

void AgaveTaskGuide::setDynamicURLParams(QString format)
{
    QList<QString> empty;
    setDynamicURLParams(format, empty);
}

void AgaveTaskGuide::setDynamicURLParams(QString format, QList<QString> subNames)
{
    dynURLFormat = format;
    urlVarNames = subNames;
}

void AgaveTaskGuide::setPostParams(QString format)
{
    QList<QString> empty;
    setPostParams(format, empty);
}

void AgaveTaskGuide::setPostParams(QString format, QList<QString> subNames)
{
    postFormat = format;
    postVarNames = subNames;
}

bool AgaveTaskGuide::usesPostParms()
{
    return !postFormat.isEmpty();
}

bool AgaveTaskGuide::usesURLParams()
{
    return !dynURLFormat.isEmpty();
}

void AgaveTaskGuide::setAgaveFullName(QString newFullName)
{
    agaveFullName = newFullName;
}

void AgaveTaskGuide::setAgavePWDparam(QString newPWDparam)
{
    agavePWDparam = newPWDparam;
}

void AgaveTaskGuide::setAgaveParamList(QStringList newParamList)
{
    agaveParamList = newParamList;
}

void AgaveTaskGuide::setAgaveInputList(QStringList newInputList)
{
    agaveInputList = newInputList;
}

QString AgaveTaskGuide::getAgaveFullName()
{
    return agaveFullName;
}

QString AgaveTaskGuide::getAgavePWDparam()
{
    return agavePWDparam;
}

QStringList AgaveTaskGuide::getAgaveParamList()
{
    return agaveParamList;
}

QStringList AgaveTaskGuide::getAgaveInputList()
{
    return agaveInputList;
}
