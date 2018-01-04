#include "agavepipebuffer.h"

AgavePipeBuffer::AgavePipeBuffer(QByteArray *oldBuffer, QObject *parent) :
    QBuffer(parent)
{
    myByteArray.clear();
    myByteArray.append(*oldBuffer);

    QBuffer::setBuffer(&myByteArray);
}
