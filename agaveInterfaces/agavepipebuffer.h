#ifndef AGAVEPIPEBUFFER_H
#define AGAVEPIPEBUFFER_H

#include <QBuffer>

class AgavePipeBuffer : public QBuffer
{
public:
    AgavePipeBuffer(QByteArray * oldBuffer, QObject * parent = NULL);
private:
    QByteArray myByteArray;
};

#endif // AGAVEPIPEBUFFER_H
