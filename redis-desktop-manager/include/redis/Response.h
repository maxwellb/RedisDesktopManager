#pragma once

#include <QtCore>

class TestResponse;

class Response
{
    friend class TestResponse;

public:
    Response();
    Response(const QByteArray &);
    ~Response(void);

    QVariant getValue();
    bool isValid();    

    void setSource(const QByteArray&);
    QByteArray source();

    void clear();

    void appendToSource(QString&);
    void appendToSource(QByteArray&);

    static QString valueToString(QVariant&, int indentLevel = 0);

    int getLoadedItemsCount();

    bool isErrorMessage() const;

    QString toString();

private:

    QByteArray responseSource;

    enum ResponseType 
    {
        Status, Error, Integer, Bulk, MultiBulk, Unknown            
    };

    int itemsCount;

    ResponseType getResponseType(const QByteArray&) const;    
    ResponseType getResponseType(const char) const;  

    bool isReplyValid(const QByteArray&);

    bool isReplyGenerallyValid(const QByteArray& );
    QString respToSimpleString(const QString &resp, int *size) const;
    QVariant respToError(const QString &resp, int *size) const;
    QVariant respToInteger(const QString &resp, int *size) const;
    QString respToBulkString(const QString &resp, int *size) const;
    QVariant respToArray(const QString &resp, int *size) const;
    QVariant parseResp(const QString &resp, int *size) const;
};
