/* Copyright (c) 2022 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "xvirustotal.h"

XVirusTotal::XVirusTotal(QObject *pParent)
    : XOnlineTools(pParent)
{

}

QJsonDocument XVirusTotal::getFileInfo(QString sHash, bool *pBNotFound)
{
    return QJsonDocument::fromJson(sendRequest(RTYPE_GETFILEINFO,sHash,nullptr,pBNotFound));
}

QJsonDocument XVirusTotal::getFileAnalyses(QString sId)
{
    return QJsonDocument::fromJson(sendRequest(RTYPE_GETFILEANALYSES,sId));
}

QString XVirusTotal::uploadFile(QIODevice *pDevice,QString sName)
{
    QString sResult;

    QJsonDocument jsDoc=QJsonDocument::fromJson(sendRequest(RTYPE_UPLOADFILE,sName,pDevice));

    if(jsDoc.isObject())
    {
        sResult=jsDoc.object()["data"].toObject()["id"].toString();
    }

    return sResult;
}

QString XVirusTotal::uploadFile(QString sFileName)
{
    QString sResult;

    QFile file;
    file.setFileName(sFileName);

    if(file.open(QIODevice::ReadOnly))
    {
        sResult=uploadFile(&file);

        file.close();
    }

    return sResult;
}

QString XVirusTotal::rescanFile(QString sHash)
{
    QString sResult;

    QJsonDocument jsDoc=QJsonDocument::fromJson(sendRequest(RTYPE_RESCANFILE,sHash));

    if(jsDoc.isObject())
    {
        sResult=jsDoc.object()["data"].toObject()["id"].toString();
    }

    return sResult;
}

QList<XVirusTotal::SCAN_RESULT> XVirusTotal::getScanResults(QString sHash,bool bShowDetected)
{
    QJsonDocument jsonDoc=getFileInfo(sHash);

    return getScanResults(&jsonDoc,bShowDetected);
}

QList<XVirusTotal::SCAN_RESULT> XVirusTotal::getScanResults(QJsonDocument *pJsonDoc,bool bShowDetected)
{
    QList<SCAN_RESULT> listResult;

    if(pJsonDoc->isObject())
    {
        QJsonObject jsonObject=pJsonDoc->object()["data"].toObject()["attributes"].toObject()["last_analysis_results"].toObject();

        QStringList slList=jsonObject.keys();
        qint32 nCount=slList.count();

        for(qint32 i=0;i<nCount;i++)
        {
            SCAN_RESULT record={};

            record.category=jsonObject[slList.at(i)].toObject()["category"].toString();
            record.engine_name=jsonObject[slList.at(i)].toObject()["engine_name"].toString();
            record.engine_version=jsonObject[slList.at(i)].toObject()["engine_version"].toString();
            record.result=jsonObject[slList.at(i)].toObject()["result"].toString();
            record.method=jsonObject[slList.at(i)].toObject()["method"].toString();
            record.engine_update=jsonObject[slList.at(i)].toObject()["engine_update"].toString();

            if((!bShowDetected)||(bShowDetected&&(record.result!="")))
            {
                listResult.append(record);
            }
        }
    }

    return listResult;
}

XVirusTotal::SCAN_INFO XVirusTotal::getScanInfo(QString sMD5)
{
    QJsonDocument jsonDoc=getFileInfo(sMD5);

    return getScanInfo(&jsonDoc);
}

XVirusTotal::SCAN_INFO XVirusTotal::getScanInfo(QJsonDocument *pJsonDoc)
{
    SCAN_INFO result={};

    if(pJsonDoc->isObject())
    {
//        QString sFirstDate=pJsonDoc->object()["data"].toObject()["attributes"].toObject()["first_submission_date"].toVariant().toString();
//        QString sLastDate=pJsonDoc->object()["data"].toObject()["attributes"].toObject()["last_analysis_date"].toVariant().toString();

        qint32 nFirstDate=pJsonDoc->object()["data"].toObject()["attributes"].toObject()["first_submission_date"].toInt();
        qint32 nLastDate=pJsonDoc->object()["data"].toObject()["attributes"].toObject()["last_analysis_date"].toInt();

        result.dtFirstScan=XBinary::valueToTime(nFirstDate,XBinary::DT_TYPE_POSIX);
        result.dtLastScan=XBinary::valueToTime(nLastDate,XBinary::DT_TYPE_POSIX);
    }

    return result;
}

QString XVirusTotal::getFileLink(QString sHash)
{
    return QString("https://www.virustotal.com/gui/file/"+sHash);
}

bool XVirusTotal::isFilePresent(QString sHash)
{
    return isPagePresent(getFileLink(sHash));
}

bool XVirusTotal::_process()
{
    bool bResult=false;

    QElapsedTimer scanTimer;
    scanTimer.start();

    getPdStruct()->pdRecordOpt.bIsValid=true;

    if((getMode()==MODE_UPLOAD)||(getMode()==MODE_RESCAN))
    {
        QString sId;

        if(getMode()==MODE_UPLOAD)
        {
            sId=uploadFile(getDevice(),getParameter());
        }
        else if(getMode()==MODE_RESCAN)
        {
            sId=rescanFile(getParameter());
        }

        if(sId!="")
        {
            while(!(getPdStruct()->bIsStop))
            {
                QJsonDocument jsDoc=QJsonDocument::fromJson(sendRequest(RTYPE_GETFILEANALYSES,sId));

                if(jsDoc.isObject())
                {
                    getPdStruct()->pdRecordOpt.sStatus=jsDoc.object()["data"].toObject()["attributes"].toObject()["status"].toString();
                }

                if((getPdStruct()->pdRecordOpt.sStatus=="")||(getPdStruct()->pdRecordOpt.sStatus=="completed"))
                {
                    break;
                }

                QThread::msleep(1000);
            }
        }
    }

    if(!(getPdStruct()->bIsStop))
    {
        getPdStruct()->pdRecordOpt.bSuccess=true;
    }

    getPdStruct()->pdRecordOpt.bFinished=true;

    emit completed(scanTimer.elapsed());

    return bResult;
}

QByteArray XVirusTotal::sendRequest(RTYPE rtype,QString sParameter,QIODevice *pDevice,bool *pBNotFound)
{
    QByteArray baResult;

    QNetworkAccessManager networkAccessManager;

    connect(&networkAccessManager,SIGNAL(sslErrors(QNetworkReply*,const QList<QSslError> &)),this,SLOT(handleSslErrors(QNetworkReply*,const QList<QSslError> &)));

    QNetworkRequest networkRequest;

    QUrl url=QUrl();
    url.setScheme("https");
    url.setHost("www.virustotal.com");

    QString sUrlPath;

    if(rtype==RTYPE_GETFILEINFO)
    {
        sUrlPath="/api/v3/files/"+sParameter;
        url.setPath(sUrlPath);
    }
    else if(rtype==RTYPE_UPLOADFILE)
    {
        if(pDevice->size()<32000000) // TODO fix 32 mb
        {
            sUrlPath="/api/v3/files";
            url.setPath(sUrlPath);
        }
        else
        {
           QJsonDocument jsDoc=QJsonDocument::fromJson(sendRequest(RTYPE_GETUPLOADLINK,""));

           if(jsDoc.isObject())
           {
//                QString sString=jsDoc.object()["data"].toVariant().toString();
                QString sString=jsDoc.object()["data"].toString();
                url.setQuery(sString);
                sUrlPath=sString.section(".com/",1,1);
           }
        }
    }
    else if(rtype==RTYPE_GETFILEANALYSES)
    {
        sUrlPath="/api/v3/analyses/"+sParameter;
        url.setPath(sUrlPath);
    }
    else if(rtype==RTYPE_RESCANFILE)
    {
        sUrlPath="/api/v3/files/"+sParameter+"/analyse";
        url.setPath(sUrlPath);
    }
    else if(rtype==RTYPE_GETUPLOADLINK)
    {
        sUrlPath="/api/v3/files/upload_url";
        url.setPath(sUrlPath);
    }

    networkRequest.setUrl(url);
    networkRequest.setRawHeader("x-apikey",getApiKey().toLatin1());

    QNetworkReply *pReply=nullptr;
    QHttpMultiPart *pMultiPart=nullptr;

    if( (rtype==RTYPE_GETFILEINFO)||
        (rtype==RTYPE_GETFILEANALYSES)||
        (rtype==RTYPE_GETUPLOADLINK))
    {
        pReply=networkAccessManager.get(networkRequest);
    }
    else if(rtype==RTYPE_UPLOADFILE)
    {
        if(sUrlPath!="")
        {
            pMultiPart=new QHttpMultiPart(QHttpMultiPart::FormDataType);

            QHttpPart filePart;

            QString sFileName=XBinary::getDeviceFileName(pDevice);

            if(sFileName=="")
            {
                if(sParameter!="")
                {
                    sFileName=sParameter;
                }
                else
                {
                    // We use MD5
                    sFileName=XBinary::getHash(XBinary::HASH_MD5,pDevice);
                }
            }

            filePart.setHeader(QNetworkRequest::ContentTypeHeader,"application/octet-stream");
            filePart.setHeader(QNetworkRequest::ContentDispositionHeader,QVariant("form-data; name=\"file\"; filename=\"" + sFileName + "\""));

            filePart.setBodyDevice(pDevice);

            pMultiPart->append(filePart);

            pReply=networkAccessManager.post(networkRequest,pMultiPart);
        }
    }

    if(pReply)
    {
        QEventLoop loop;

        connect(pReply,&QNetworkReply::finished,&loop,&QEventLoop::quit);
        connect(pReply,&QNetworkReply::downloadProgress,this,&XOnlineTools::_downloadProgress,Qt::DirectConnection);
        connect(pReply,&QNetworkReply::uploadProgress,this,&XOnlineTools::_uploadProgress,Qt::DirectConnection);
        connect(pReply,&QNetworkReply::finished,this,&XOnlineTools::_finished,Qt::DirectConnection);

        loop.exec();

        if(pReply->error()==QNetworkReply::NoError)
        {
            baResult=pReply->readAll();

            if(pBNotFound)
            {
                *pBNotFound=false;
            }
        }
        else if(pReply->error()==QNetworkReply::ContentNotFoundError)
        {
            if(pBNotFound)
            {
                *pBNotFound=true;
            }
        }
        else
        {
            emit errorMessage(pReply->errorString());
        #ifdef QT_DEBUG
            qDebug("%s",pReply->errorString().toLatin1().data());
        #endif
        }
    }

    if(pMultiPart)
    {
        delete pMultiPart;
    }

#ifdef QT_DEBUG
    qDebug(baResult.data());
#endif

    return baResult;
}
