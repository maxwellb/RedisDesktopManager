#include "Response.h"
#include "RedisException.h"

Response::Response()
    : responseSource(""), itemsCount(0)
{
}

Response::Response(const QByteArray & src)
    : responseSource(src), itemsCount(0)
{
}

Response::~Response(void)
{
}

void Response::setSource(const QByteArray& src)
{
    responseSource = src;
}

void Response::clear()
{
    responseSource.clear();
    itemsCount = 0;
}

QByteArray Response::source()
{
    return responseSource;
}

void Response::appendToSource(QString& src)
{
    responseSource.append(src);
}

void Response::appendToSource(QByteArray& src)
{
    responseSource.append(src);
}

QString Response::toString()
{
    return responseSource.left(1500);
}

QVariant Response::getValue()
{
    if (responseSource.isEmpty()) {
        return QVariant();
    }

    QVariant  parsedResponse;

    try {
        QString resp = QString(responseSource);
        parsedResponse = parseResp(resp, 0);

    } catch (RedisException &e) {
        parsedResponse = QVariant(QStringList() << e.what());
    }

    return parsedResponse;
}    

QVariant Response::parseResp(const QString & resp, int* size = 0) const
{
    QVariant parsedResponse;
    int parsedSize = 0;

    if (resp.length() < 1)
        parsedResponse = QVariant();
    else if (resp.at(0) == QChar('+'))
        parsedResponse = QVariant(respToSimpleString(resp, &parsedSize));
    else if (resp.at(0) == QChar('-'))
        parsedResponse = QVariant(respToError(resp, &parsedSize));
    else if (resp.at(0) == QChar(':'))
        parsedResponse = QVariant(respToInteger(resp, &parsedSize));
    else if (resp.at(0) == QChar('$'))
        parsedResponse = QVariant(respToBulkString(resp, &parsedSize));
    else if (resp.at(0) == QChar('*'))
        parsedResponse = QVariant(respToArray(resp, &parsedSize));
    else
        parsedResponse = QVariant();

    if (size > 0)
        *size = parsedSize;
    return parsedResponse;
}

QString Response::respToSimpleString(const QString & resp, int* size = 0) const
{
    QString result;

    result = resp.mid(1, resp.indexOf("\r\n") - 1);
    if (size > 0)
        *size = resp.indexOf("\r\n") + 2;

    return result;
}

QVariant Response::respToError(const QString & resp, int* size = 0) const
{
    QVariant result;

    result = QVariant(resp.mid(0, resp.indexOf("\r\n")));
    if (size > 0)
        *size = resp.indexOf("\r\n") + 2;

    return result;
}

QVariant Response::respToInteger(const QString & resp, int* size = 0) const
{
    QVariant result;
    int parsedInt;
    bool ok;

    parsedInt = resp.mid(1, resp.indexOf("\r\n") - 1).toInt(&ok);
    result = ok ? QVariant(parsedInt) : QVariant();
    if (size > 0)
        *size = resp.indexOf("\r\n") + 2;

    return result;
}

QString Response::respToBulkString(const QString & resp, int* size = 0) const
{
    QString result;

    int len = resp.mid(1, resp.indexOf("\r\n")).toInt();
    QString right = resp.mid(resp.indexOf("\r\n") + 2);
    result = len >= 0 && right.endsWith("\r\n") ? right.left(len) : QString();
    if (size > 0)
    {
        if (len < 0)
            *size = resp.indexOf("\r\n") + 2;
        else
            *size = resp.indexOf("\r\n") + len + 4;
    }
    return result;
}

QVariant Response::respToArray(const QString & resp, int* size = 0) const
{
    QVariant result;
    int totalSize = 0;

    int len = resp.mid(1, resp.indexOf("\r\n")).toInt();
    if (len < 0)
    {
        result = QVariant();
        totalSize = resp.indexOf("\r\n") + 2;
    }
    else
    {
        QVariantList list = QVariantList();
        QString nextLines = QString(resp.mid(resp.indexOf("\r\n") + 2));
        for (int i = 0; !nextLines.isNull() && i < len; ++i)
        {
            QVariant current;
            int parsedSize;

            current = parseResp(nextLines, &parsedSize);
            list.append(QVariant(current));

            nextLines = QString(nextLines.mid(parsedSize));
			totalSize += parsedSize;
        }
		result = QVariant(list);
    }


    if (size > 0)
        *size = totalSize;
    return result;
}

Response::ResponseType Response::getResponseType(const QByteArray & r) const
{
    return getResponseType(r.at(0));
}

Response::ResponseType Response::getResponseType(const char typeChar) const
{
    if (typeChar == '+') return Status;
    if (typeChar == '-') return Error;
    if (typeChar == ':') return Integer;
    if (typeChar == '$') return Bulk;
    if (typeChar == '*') return MultiBulk;

    return Unknown;
}

bool Response::isValid()
{
    return isReplyValid(responseSource);
}

bool Response::isReplyValid(const QByteArray & responseString)
{
    if (responseString.isEmpty()) 
    {
        return false;
    }

    ResponseType type = getResponseType(responseString);

    switch (type)
    {
        case Status:
        case Error:        
        case Unknown:
        default:
            return isReplyGenerallyValid(responseString);
    }    
}

bool Response::isReplyGenerallyValid(const QByteArray& r)
{
    return r.endsWith("\r\n");
}

QString Response::valueToString(QVariant& value, int indentLevel)
{
	QString result;
    QString indent = "";
    for (int i = 0; i < indentLevel; ++i)
    {
        indent.append("\t");
    }
    if (value.isNull()) 
    {
        result = QString("null");
    }
    else if (value.type() == QMetaType::Bool)
    {
        result = QString(value.toBool() ? "true" : "false");
    }
    else if (value.type() == QMetaType::QString)
    {
        result = indentLevel > 0 ? "\"" + value.toString() + "\"" : value.toString();
    }
    else if (value.type() == QVariant::StringList)
    {
		if (indentLevel > 0)
		{
			result = "[";
			result.append(QStringList(value.toStringList()).replaceInStrings(QRegExp("(.*)"), "\"\\1\"").join(", "));
			result.append("]");
		}
		else
		{
			result = value.toStringList().join("\r\n");
		}
    }
    else if (value.type() == QMetaType::QVariantList || value.canConvert(QMetaType::QVariantList))
    {
		QVariantList list = value.value<QVariantList>();
        result = "[\r\n";
		QStringList strings;
		for (QVariant item : list)
		{
            QString stringRep = valueToString(item, indentLevel + 1);
			strings.append(stringRep);
		}
        result.append(strings.join(", \r\n"));
        result.append("\r\n").append(indent).append("]");
    }
	else
	{
		result = value.toString();
	}

    return indent + result;
}

int Response::getLoadedItemsCount()
{
    return itemsCount;
}

bool Response::isErrorMessage() const
{
    return getResponseType(responseSource) == Error
        && responseSource.startsWith("-ERR");

}
