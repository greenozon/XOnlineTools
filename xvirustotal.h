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
#ifndef XVIRUSTOTAL_H
#define XVIRUSTOTAL_H

#include "xonlinetools.h"

class XVirusTotal : public XOnlineTools
{
    Q_OBJECT
public:
    enum RTYPE
    {
        RTYPE_UNKNOWN=0,
        RTYPE_GETFILEINFO,
        RTYPE_UPLOADFILE,
        RTYPE_GETFILEANALYSES
    };

    struct SCAN_RESULT
    {
        QString category;
        QString engine_name;
        QString engine_version;
        QString result;
        QString method;
        QString engine_update;
    };

    explicit XVirusTotal(QObject *pParent=nullptr);

    QJsonDocument getFileInfo(QString sMD5,bool *pBNotFound=nullptr);
    QJsonDocument getFileAnalyses(QString sId);
    bool uploadFile(QIODevice *pDevice,QString sName="");
    bool uploadFile(QString sFileName);
    QList<SCAN_RESULT> getScanResults(QString sMD5);
    static QList<SCAN_RESULT> getScanResults(QJsonDocument *pJsonDoc);
    QString getScanInfo(QString sMD5);
    static QString getScanInfo(QJsonDocument *pJsonDoc);

protected:
    virtual bool _process();

private:
    QByteArray sendRequest(RTYPE rtype,QString sParameter,QIODevice *pDevice=nullptr,bool *pBNotFound=nullptr);
};

#endif // XVIRUSTOTAL_H